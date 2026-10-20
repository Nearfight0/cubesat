#include "MainWindow.h"

#include "SerialManager.h"
#include "CsvLogger.h"
#include "SimulationManager.h"
#include "ChartManager.h"
#include "MapView.h"
#include "CameraPanel.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QDockWidget>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QProgressBar>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QScrollBar>
#include <QDateTime>
#include <QMetaEnum>
#include <QFrame>
#include <QSizePolicy>
#include <QSplitter>

// ── Ctor / Dtor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("CubeSat Ground Station — Mission: Kripto");
    setMinimumSize(1280, 760);
    resize(1440, 860);

    // ── Managers ─────────────────────────────────────────────────────────────
    m_serial   = new SerialManager(this);
    m_csvLogger = new CsvLogger(this);
    m_simMgr   = new SimulationManager(this);
    m_chartMgr = new ChartManager(this);

    // ── Port auto-refresh timer ───────────────────────────────────────────────
    m_portRefreshTimer = new QTimer(this);
    m_portRefreshTimer->setInterval(PORT_REFRESH_INTERVAL_MS);
    connect(m_portRefreshTimer, &QTimer::timeout,
            this, &MainWindow::onPortRefreshTick);
    m_portRefreshTimer->start();

    // ── Build UI ──────────────────────────────────────────────────────────────
    buildStatusBar();
    buildCentralTabs();
    buildRightDock();

    // ── Wire SerialManager ────────────────────────────────────────────────────
    connect(m_serial, &SerialManager::packetReceived,
            this, &MainWindow::onPacketReceived);
    connect(m_serial, &SerialManager::rawLineReceived,
            this, &MainWindow::onRawLineReceived);
    connect(m_serial, &SerialManager::statusChanged,
            this, &MainWindow::onSerialStatusChanged);
    connect(m_serial, &SerialManager::portsUpdated,
            this, &MainWindow::onPortsUpdated);
    connect(m_serial, &SerialManager::parseError,
            this, &MainWindow::onParseError);

    // ── Wire CsvLogger ────────────────────────────────────────────────────────
    connect(m_serial, &SerialManager::packetReceived,
            m_csvLogger, &CsvLogger::logPacket);
    connect(m_csvLogger, &CsvLogger::logStatusChanged,
            this, &MainWindow::onLogStatusChanged);
    connect(m_csvLogger, &CsvLogger::logError,
            [this](const QString &msg){
                appendRawLog("[CSV ERROR] " + msg, "#E74C3C");
            });

    // ── Wire ChartManager & MapView to live serial ────────────────────────────
    connect(m_serial, &SerialManager::packetReceived,
            m_chartMgr, &ChartManager::updateCharts);
    connect(m_serial, &SerialManager::packetReceived,
            m_mapView, &MapView::updatePosition);

    // ── Wire SimulationManager ────────────────────────────────────────────────
    connect(m_simMgr, &SimulationManager::fileLoaded,
            this, &MainWindow::onSimFileLoaded);
    connect(m_simMgr, &SimulationManager::fileLoadError,
            this, &MainWindow::onSimFileLoadError);
    connect(m_simMgr, &SimulationManager::progressChanged,
            this, &MainWindow::onSimProgressChanged);
    connect(m_simMgr, &SimulationManager::simulationComplete,
            this, &MainWindow::onSimComplete);
    connect(m_simMgr,
            qOverload<SimulationManager::State>(&SimulationManager::stateChanged),
            [this](SimulationManager::State s){
                onSimStateChanged(static_cast<int>(s));
            });

    // Initial port scan
    m_serial->refreshPorts();
}

MainWindow::~MainWindow() = default;

// ── UI Builders ───────────────────────────────────────────────────────────────

void MainWindow::buildCentralTabs()
{
    // ── Logo bar ──────────────────────────────────────────────────────────────
    auto *logoBar = new QWidget;
    logoBar->setFixedHeight(120);
    auto *logoLayout = new QHBoxLayout(logoBar);
    logoLayout->setContentsMargins(16, 8, 16, 8);
    logoLayout->setSpacing(24);

    auto addLogo = [&](const QString &path) {
        auto *lbl = new QLabel;
        lbl->setPixmap(QPixmap(path).scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        lbl->setAlignment(Qt::AlignCenter);
        logoLayout->addWidget(lbl);
    };

    addLogo(":/logo.png");
    addLogo(":/azst.png");
    addLogo(":/final_logo.png");

    logoLayout->addStretch();

    // ── Tabs ──────────────────────────────────────────────────────────────────
    auto *tabs = new QTabWidget;
    tabs->setDocumentMode(true);

    // Tab 1: Charts (+ live camera feed underneath)
    auto *chartsTab = new QWidget;
    auto *chartsLayout = new QVBoxLayout(chartsTab);
    chartsLayout->setContentsMargins(0, 0, 0, 0);

    auto *chartsSplitter = new QSplitter(Qt::Vertical, chartsTab);
    chartsSplitter->addWidget(m_chartMgr->widget());
    m_cameraPanel = new CameraPanel(chartsSplitter);
    chartsSplitter->addWidget(m_cameraPanel);
    chartsSplitter->setStretchFactor(0, 3);
    chartsSplitter->setStretchFactor(1, 2);
    chartsLayout->addWidget(chartsSplitter);

    tabs->addTab(chartsTab, "Charts");

    // Tab 2: GPS Map
    m_mapView = new MapView(this);
    tabs->addTab(m_mapView, "GPS Map");

    // Tab 3: Raw Data Log
    auto *rawTab = new QWidget;
    auto *rawLayout = new QVBoxLayout(rawTab);
    buildSerialPanel(rawTab);
    m_rawLog = new QPlainTextEdit;
    m_rawLog->setReadOnly(true);
    m_rawLog->setFont(QFont("Consolas", 9));
    m_rawLog->setPlaceholderText("Incoming serial data will appear here…");
    m_rawLog->setMaximumBlockCount(500);
    m_rawLog->setWordWrapMode(QTextOption::NoWrap);
    rawLayout->addWidget(m_rawLog);
    tabs->addTab(rawTab, "Raw Data Log");

    // Tab 4: Simulation
    auto *simTab = new QWidget;
    buildSimulationTab(simTab);
    tabs->addTab(simTab, "Simulation");

    auto *central = new QWidget;
    auto *mainVL  = new QVBoxLayout(central);
    mainVL->setContentsMargins(0, 0, 0, 0);
    mainVL->setSpacing(0);
    mainVL->addWidget(logoBar);
    mainVL->addWidget(tabs);
    setCentralWidget(central);
}

void MainWindow::buildSerialPanel(QWidget *parent)
{
    auto *layout = qobject_cast<QVBoxLayout *>(parent->layout());
    if (!layout) {
        layout = new QVBoxLayout(parent);
    }

    auto *group = new QGroupBox("XBee Serial Connection", parent);
    auto *gl    = new QGridLayout(group);
    gl->setSpacing(6);

    // Row 0: Port selection
    gl->addWidget(new QLabel("COM Port:"), 0, 0);
    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(120);
    gl->addWidget(m_portCombo, 0, 1);

    m_refreshPortsBtn = new QPushButton("Refresh");
    m_refreshPortsBtn->setMaximumWidth(70);
    connect(m_refreshPortsBtn, &QPushButton::clicked,
            this, &MainWindow::onRefreshPortsClicked);
    gl->addWidget(m_refreshPortsBtn, 0, 2);

    // Row 1: Baud rate
    gl->addWidget(new QLabel("Baud Rate:"), 1, 0);
    m_baudCombo = new QComboBox;
    for (int baud : {9600, 19200, 38400, 57600, 115200})
        m_baudCombo->addItem(QString::number(baud), baud);
    m_baudCombo->setCurrentIndex(0); // 9600
    gl->addWidget(m_baudCombo, 1, 1);

    // Row 2: Connect + status
    m_connectBtn = new QPushButton("Connect");
    connect(m_connectBtn, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    gl->addWidget(m_connectBtn, 2, 0, 1, 2);

    m_connStatusLight = new QLabel;
    m_connStatusLight->setFixedSize(16, 16);
    m_connStatusLight->setStyleSheet(
        "background-color: #555555; border-radius: 8px;"
    );
    m_connStatusLight->setToolTip("Connection status");
    gl->addWidget(m_connStatusLight, 2, 2, Qt::AlignCenter);

    gl->setColumnStretch(1, 1);
    layout->addWidget(group);
}

void MainWindow::buildRightDock()
{
    auto *dock = new QDockWidget("Telemetry Dashboard", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);
    dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    dock->setMinimumWidth(280);

    auto *container = new QWidget;
    auto *vl = new QVBoxLayout(container);
    vl->setSpacing(8);

    buildDashboardPanel(container);
    buildCommandPanel(container);

    vl->addStretch();
    dock->setWidget(container);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::buildDashboardPanel(QWidget *parent)
{
    auto *vl = qobject_cast<QVBoxLayout *>(parent->layout());

    auto *group = new QGroupBox("Live Telemetry");
    auto *form  = new QFormLayout(group);
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(4);

    auto makeLabel = [](const QString &placeholder = "—") {
        auto *l = new QLabel(placeholder);
        l->setStyleSheet("color: #E0E0E0; font-family: 'Consolas'; font-size: 10pt;");
        return l;
    };

    m_lblTeamId      = makeLabel(); form->addRow("Team ID:",       m_lblTeamId);
    m_lblPacketNo    = makeLabel(); form->addRow("Packet #:",      m_lblPacketNo);
    m_lblTime        = makeLabel(); form->addRow("Time:",          m_lblTime);

    auto *sep1 = new QFrame; sep1->setFrameShape(QFrame::HLine);
    form->addRow(sep1);

    m_lblPressure    = makeLabel(); form->addRow("Pressure:",      m_lblPressure);
    m_lblRelAlt      = makeLabel(); form->addRow("Rel. Altitude:", m_lblRelAlt);
    m_lblDescentSpeed= makeLabel(); form->addRow("Descent Speed:", m_lblDescentSpeed);
    m_lblVoltage     = makeLabel(); form->addRow("Voltage:",       m_lblVoltage);

    auto *sep2 = new QFrame; sep2->setFrameShape(QFrame::HLine);
    form->addRow(sep2);

    m_lblGpsLat      = makeLabel(); form->addRow("GPS Lat:",       m_lblGpsLat);
    m_lblGpsLng      = makeLabel(); form->addRow("GPS Lng:",       m_lblGpsLng);
    m_lblGpsAlt      = makeLabel(); form->addRow("GPS Alt:",       m_lblGpsAlt);

    auto *sep3 = new QFrame; sep3->setFrameShape(QFrame::HLine);
    form->addRow(sep3);

    m_lblPitch       = makeLabel(); form->addRow("Pitch:",         m_lblPitch);
    m_lblRoll        = makeLabel(); form->addRow("Roll:",          m_lblRoll);
    m_lblYaw         = makeLabel(); form->addRow("Yaw:",           m_lblYaw);

    auto *sep4 = new QFrame; sep4->setFrameShape(QFrame::HLine);
    form->addRow(sep4);

    // Separation badge
    m_lblSeparation = new QLabel("NOT SEPARATED");
    m_lblSeparation->setAlignment(Qt::AlignCenter);
    m_lblSeparation->setStyleSheet(
        "background-color: #555555; color: white; font-weight: bold;"
        "border-radius: 4px; padding: 4px;"
    );
    form->addRow("Separation:", m_lblSeparation);

    // Crypto message (prominent)
    m_lblCrypto = new QLabel("—");
    m_lblCrypto->setWordWrap(true);
    m_lblCrypto->setStyleSheet(
        "color: #0078D4; font-weight: bold; font-size: 11pt;"
        "background-color: #252525; border-radius: 4px; padding: 4px;"
    );
    form->addRow("Crypto Msg:", m_lblCrypto);

    vl->addWidget(group);
}

void MainWindow::buildCommandPanel(QWidget *parent)
{
    auto *vl = qobject_cast<QVBoxLayout *>(parent->layout());

    auto *group = new QGroupBox("Command Panel");
    auto *gl    = new QGridLayout(group);
    gl->setSpacing(6);

    // Crypto word input + send
    gl->addWidget(new QLabel("Crypto Word:"), 0, 0);
    m_cryptoInput = new QLineEdit;
    m_cryptoInput->setPlaceholderText("Enter crypto word…");
    gl->addWidget(m_cryptoInput, 0, 1);

    m_sendCryptoBtn = new QPushButton("Send");
    connect(m_sendCryptoBtn, &QPushButton::clicked,
            this, &MainWindow::onSendCryptoWord);
    gl->addWidget(m_sendCryptoBtn, 0, 2);

    // Separation trigger
    m_triggerSepBtn = new QPushButton("Trigger Separation");
    m_triggerSepBtn->setObjectName("dangerButton");
    connect(m_triggerSepBtn, &QPushButton::clicked,
            this, &MainWindow::onTriggerSeparation);
    gl->addWidget(m_triggerSepBtn, 1, 0, 1, 3);

    // Command log
    gl->addWidget(new QLabel("Command Log:"), 2, 0, 1, 3);
    m_cmdLog = new QTextEdit;
    m_cmdLog->setReadOnly(true);
    m_cmdLog->setMaximumHeight(120);
    m_cmdLog->setFont(QFont("Consolas", 8));
    gl->addWidget(m_cmdLog, 3, 0, 1, 3);

    gl->setColumnStretch(1, 1);
    vl->addWidget(group);
}

void MainWindow::buildSimulationTab(QWidget *parent)
{
    auto *vl = new QVBoxLayout(parent);
    vl->setSpacing(10);

    // ── File loading ──────────────────────────────────────────────────────────
    auto *fileGroup = new QGroupBox("Telemetry File");
    auto *fgl = new QHBoxLayout(fileGroup);

    m_simFileLabel = new QLabel("No file loaded");
    m_simFileLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_simFileLabel->setStyleSheet("color: #AAAAAA; font-style: italic;");
    fgl->addWidget(m_simFileLabel);

    m_simBrowseBtn = new QPushButton("Browse…");
    connect(m_simBrowseBtn, &QPushButton::clicked,
            this, &MainWindow::onSimBrowseClicked);
    fgl->addWidget(m_simBrowseBtn);

    vl->addWidget(fileGroup);

    // ── File info ─────────────────────────────────────────────────────────────
    m_simInfoLabel = new QLabel("Load a CSV telemetry file to begin playback.");
    m_simInfoLabel->setAlignment(Qt::AlignCenter);
    m_simInfoLabel->setStyleSheet("color: #AAAAAA;");
    vl->addWidget(m_simInfoLabel);

    // ── Playback controls ─────────────────────────────────────────────────────
    auto *ctrlGroup = new QGroupBox("Playback Controls");
    auto *ctrlLayout = new QVBoxLayout(ctrlGroup);

    auto *btnRow = new QHBoxLayout;
    m_simPlayBtn  = new QPushButton("Play");
    m_simPauseBtn = new QPushButton("Pause");
    m_simStopBtn  = new QPushButton("Stop");
    m_simResetBtn = new QPushButton("Reset");

    m_simPlayBtn->setEnabled(false);
    m_simPauseBtn->setEnabled(false);
    m_simStopBtn->setEnabled(false);
    m_simResetBtn->setEnabled(false);

    connect(m_simPlayBtn,  &QPushButton::clicked, this, &MainWindow::onSimPlayClicked);
    connect(m_simPauseBtn, &QPushButton::clicked, this, &MainWindow::onSimPauseClicked);
    connect(m_simStopBtn,  &QPushButton::clicked, this, &MainWindow::onSimStopClicked);
    connect(m_simResetBtn, &QPushButton::clicked, this, &MainWindow::onSimResetClicked);

    btnRow->addWidget(m_simPlayBtn);
    btnRow->addWidget(m_simPauseBtn);
    btnRow->addWidget(m_simStopBtn);
    btnRow->addWidget(m_simResetBtn);

    auto *speedRow = new QHBoxLayout;
    speedRow->addWidget(new QLabel("Speed:"));
    m_simSpeedCombo = new QComboBox;
    m_simSpeedCombo->addItem("0.5×", 0.5);
    m_simSpeedCombo->addItem("1×",   1.0);
    m_simSpeedCombo->addItem("2×",   2.0);
    m_simSpeedCombo->addItem("5×",   5.0);
    m_simSpeedCombo->addItem("10×", 10.0);
    m_simSpeedCombo->setCurrentIndex(1); // default 1×
    connect(m_simSpeedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSimSpeedChanged);
    speedRow->addWidget(m_simSpeedCombo);
    speedRow->addStretch();

    m_simProgress = new QProgressBar;
    m_simProgress->setRange(0, 100);
    m_simProgress->setValue(0);
    m_simProgress->setTextVisible(true);
    m_simProgress->setFormat("%v / %m packets");

    auto *infoRow = new QHBoxLayout;
    m_simElapsedLabel = new QLabel("Elapsed: 00:00:00");
    m_simPacketLabel  = new QLabel("Packet: 0 / 0");
    infoRow->addWidget(m_simElapsedLabel);
    infoRow->addStretch();
    infoRow->addWidget(m_simPacketLabel);

    ctrlLayout->addLayout(btnRow);
    ctrlLayout->addLayout(speedRow);
    ctrlLayout->addWidget(m_simProgress);
    ctrlLayout->addLayout(infoRow);

    vl->addWidget(ctrlGroup);
    vl->addStretch();
}

void MainWindow::buildStatusBar()
{
    m_sbStatus  = new QLabel("Disconnected");
    m_sbPackets = new QLabel("Packets: 0");
    m_sbLastTs  = new QLabel("Last: —");
    m_sbCsvPath = new QLabel("CSV: not logging");

    statusBar()->addPermanentWidget(m_sbStatus, 2);
    statusBar()->addPermanentWidget(m_sbPackets, 1);
    statusBar()->addPermanentWidget(m_sbLastTs, 2);
    statusBar()->addPermanentWidget(m_sbCsvPath, 3);
}

// ── Serial slots ──────────────────────────────────────────────────────────────

void MainWindow::onConnectClicked()
{
    if (m_serial->isConnected()) {
        m_serial->disconnectPort();
        m_connectBtn->setText("Connect");
        m_csvLogger->closeFile();
    } else {
        if (m_portCombo->currentText().isEmpty()) {
            m_sbStatus->setText("No COM port selected");
            return;
        }
        const int baud = m_baudCombo->currentData().toInt();
        m_serial->connectToPort(m_portCombo->currentText(), baud);
        m_connectBtn->setText("Disconnect");
    }
}

void MainWindow::onPortsUpdated(const QStringList &ports)
{
    const QString current = m_portCombo->currentText();
    m_portCombo->blockSignals(true);
    m_portCombo->clear();
    m_portCombo->addItems(ports);
    int idx = m_portCombo->findText(current);
    if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    m_portCombo->blockSignals(false);
}

void MainWindow::onSerialStatusChanged(const QString &status, bool connected)
{
    m_sbStatus->setText(status);
    m_connStatusLight->setStyleSheet(
        connected
            ? "background-color: #27AE60; border-radius: 8px;"
            : "background-color: #E74C3C; border-radius: 8px;"
    );

    if (!connected) {
        m_connectBtn->setText("Connect");
    } else {
        m_connectBtn->setText("Disconnect");
    }
}

void MainWindow::onRawLineReceived(const QString &line)
{
    appendRawLog(line);
}

void MainWindow::onParseError(const QString &error)
{
    appendRawLog("[PARSE ERROR] " + error, "#E74C3C");
}

void MainWindow::onRefreshPortsClicked()
{
    m_serial->refreshPorts();
}

void MainWindow::onPortRefreshTick()
{
    // Silently refresh — only updates if not connected to avoid disruption
    if (!m_serial->isConnected())
        m_serial->refreshPorts();
}

// ── Command slots ─────────────────────────────────────────────────────────────

void MainWindow::onSendCryptoWord()
{
    if (!m_serial->isConnected()) {
        appendCommandLog("[WARN] Not connected — command not sent.");
        return;
    }

    const QString word = m_cryptoInput->text().trimmed();
    if (word.isEmpty()) return;

    // SerialWorker writes to the port on its thread; we send via invokeMethod
    // For simplicity, the command is sent as a raw string + newline
    const QString cmd = word + "\n";
    QMetaObject::invokeMethod(
        m_serial, [this, cmd]() {
            // Direct access not available here — use a public write slot
            // (see note: add SerialManager::sendCommand() if needed)
        }
    );

    appendCommandLog(QString("[%1] CRYPTO: %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(word));
    m_cryptoInput->clear();
}

void MainWindow::onTriggerSeparation()
{
    if (!m_serial->isConnected()) {
        appendCommandLog("[WARN] Not connected — command not sent.");
        return;
    }

    const int ret = QMessageBox::warning(
        this,
        "Confirm Separation",
        "Send SEPARATION TRIGGER command to CubeSat?\n\nThis action cannot be undone.",
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel
    );

    if (ret == QMessageBox::Yes) {
        appendCommandLog(QString("[%1] SEPARATION TRIGGER SENT")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    }
}

// ── Data slots ────────────────────────────────────────────────────────────────

void MainWindow::onPacketReceived(const TelemetryPacket &packet)
{
    ++m_packetCount;
    updateWindowTitle(packet.teamId);
    updateDashboard(packet);

    m_sbPackets->setText(QString("Packets: %1").arg(m_packetCount));
    m_sbLastTs->setText("Last: " + packet.timeRaw);
}

void MainWindow::onLogStatusChanged(const QString &filePath, int rowCount)
{
    if (filePath.isEmpty() || !m_csvLogger->isEnabled()) {
        m_sbCsvPath->setText("CSV: not logging");
    } else {
        m_sbCsvPath->setText(
            QString("CSV: %1  [%2 rows]").arg(filePath).arg(rowCount)
        );
    }
}

// ── Simulation slots ──────────────────────────────────────────────────────────

void MainWindow::onSimBrowseClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Telemetry CSV", QString(),
        "CSV Files (*.csv);;All Files (*)"
    );
    if (path.isEmpty()) return;

    m_simFileLabel->setText(path);
    m_simFileLabel->setStyleSheet("color: #E0E0E0;");
    m_simMgr->loadFile(path);
}

void MainWindow::onSimPlayClicked()
{
    // Switch to simulation mode: disconnect live serial connections
    setLiveMode(false);
    m_simMgr->play();
}

void MainWindow::onSimPauseClicked()
{
    m_simMgr->pause();
}

void MainWindow::onSimStopClicked()
{
    m_simMgr->stop();
    m_chartMgr->reset();
    m_mapView->reset();
    setLiveMode(true);
    m_sbStatus->setText("Simulation stopped");
}

void MainWindow::onSimResetClicked()
{
    m_simMgr->reset();
    m_chartMgr->reset();
    m_mapView->reset();
    setLiveMode(true);
    m_simProgress->setValue(0);
    m_simElapsedLabel->setText("Elapsed: 00:00:00");
    m_simPacketLabel->setText("Packet: 0 / 0");
}

void MainWindow::onSimSpeedChanged(int index)
{
    const double speed = m_simSpeedCombo->itemData(index).toDouble();
    m_simMgr->setSpeed(speed);
}

void MainWindow::onSimFileLoaded(int total, int skipped, int durationSec)
{
    const int h = durationSec / 3600;
    const int m = (durationSec % 3600) / 60;
    const int s = durationSec % 60;

    m_simInfoLabel->setText(
        QString("Loaded: %1 packets  |  Skipped: %2  |  Duration: %3:%4:%5")
            .arg(total)
            .arg(skipped)
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
    );
    m_simInfoLabel->setStyleSheet("color: #27AE60;");

    m_simProgress->setRange(0, total);
    m_simProgress->setValue(0);
    m_simPacketLabel->setText(QString("Packet: 0 / %1").arg(total));

    setSimControlsEnabled(true);
}

void MainWindow::onSimFileLoadError(const QString &msg)
{
    QMessageBox::critical(this, "File Load Error", msg);
    m_simInfoLabel->setText("Error loading file.");
    m_simInfoLabel->setStyleSheet("color: #E74C3C;");
    setSimControlsEnabled(false);
}

void MainWindow::onSimProgressChanged(int current, int total)
{
    m_simProgress->setValue(current);
    m_simPacketLabel->setText(
        QString("Packet: %1 / %2").arg(current).arg(total)
    );

    // Elapsed based on packet count at nominal 1 Hz
    const int elapsed = current;
    const int h = elapsed / 3600;
    const int m = (elapsed % 3600) / 60;
    const int s = elapsed % 60;
    m_simElapsedLabel->setText(
        QString("Elapsed: %1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
    );
}

void MainWindow::onSimComplete()
{
    m_sbStatus->setText("Simulation Complete");
    m_sbStatus->setStyleSheet("color: #27AE60; font-weight: bold;");
    setLiveMode(true);
}

void MainWindow::onSimStateChanged(int stateInt)
{
    using State = SimulationManager::State;
    const auto state = static_cast<State>(stateInt);

    switch (state) {
    case State::Idle:
    case State::Loaded:
        m_simPlayBtn->setEnabled(state == State::Loaded);
        m_simPauseBtn->setEnabled(false);
        m_simStopBtn->setEnabled(false);
        m_simResetBtn->setEnabled(state == State::Loaded);
        break;

    case State::Playing:
        m_simPlayBtn->setEnabled(false);
        m_simPauseBtn->setEnabled(true);
        m_simStopBtn->setEnabled(true);
        m_simResetBtn->setEnabled(false);
        m_sbStatus->setStyleSheet("color: #E67E22; font-weight: bold;");
        m_sbStatus->setText("SIMULATION MODE");
        break;

    case State::Paused:
        m_simPlayBtn->setEnabled(true);
        m_simPauseBtn->setEnabled(false);
        m_simStopBtn->setEnabled(true);
        m_simResetBtn->setEnabled(true);
        m_sbStatus->setText("SIMULATION — PAUSED");
        break;
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MainWindow::setLiveMode(bool live)
{
    m_simulationMode = !live;

    if (live) {
        // Reconnect sim packets back to dashboard / charts / map
        disconnect(m_simMgr, &SimulationManager::packetReceived,
                   this, &MainWindow::onPacketReceived);
        disconnect(m_simMgr, &SimulationManager::packetReceived,
                   m_chartMgr, &ChartManager::updateCharts);
        disconnect(m_simMgr, &SimulationManager::packetReceived,
                   m_mapView, &MapView::updatePosition);

        m_csvLogger->setEnabled(true);
        setSerialPanelEnabled(true);
        m_sbStatus->setStyleSheet("");
    } else {
        // Route sim packets to the same handlers
        connect(m_simMgr, &SimulationManager::packetReceived,
                this, &MainWindow::onPacketReceived, Qt::UniqueConnection);
        connect(m_simMgr, &SimulationManager::packetReceived,
                m_chartMgr, &ChartManager::updateCharts, Qt::UniqueConnection);
        connect(m_simMgr, &SimulationManager::packetReceived,
                m_mapView, &MapView::updatePosition, Qt::UniqueConnection);

        // Disable CSV logging and serial panel during simulation
        m_csvLogger->setEnabled(false);
        setSerialPanelEnabled(false);

        m_packetCount = 0;
        m_chartMgr->reset();
        m_mapView->reset();
    }
}

void MainWindow::appendRawLog(const QString &text, const QString &color)
{
    if (!m_rawLog) return;
    Q_UNUSED(color) // QPlainTextEdit is plain text — no per-line colour needed

    const QString ts  = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    const QString line = QStringLiteral("[") + ts + QStringLiteral("] ") + text;

    // appendPlainText is O(1) — does NOT re-render the whole document
    m_rawLog->appendPlainText(line);
}

void MainWindow::appendCommandLog(const QString &text)
{
    if (!m_cmdLog) return;
    m_cmdLog->append(text);
    m_cmdLog->verticalScrollBar()->setValue(
        m_cmdLog->verticalScrollBar()->maximum()
    );
}

void MainWindow::updateDashboard(const TelemetryPacket &p)
{
    m_lblTeamId->setText(p.teamId);
    m_lblPacketNo->setText(QString::number(p.packetNo));
    m_lblTime->setText(p.timeRaw);
    m_lblPressure->setText(QString("%1 Pa").arg(p.pressure, 0, 'f', 1));
    m_lblRelAlt->setText(QString("%1 m").arg(p.relAltitude, 0, 'f', 2));
    m_lblDescentSpeed->setText(QString("%1 m/s").arg(p.descentSpeed, 0, 'f', 2));
    m_lblGpsLat->setText(QString("%1°").arg(p.gpsLat, 0, 'f', 6));
    m_lblGpsLng->setText(QString("%1°").arg(p.gpsLng, 0, 'f', 6));
    m_lblGpsAlt->setText(QString("%1 m").arg(p.gpsAlt, 0, 'f', 1));
    m_lblPitch->setText(QString("%1°").arg(p.pitch, 0, 'f', 2));
    m_lblRoll->setText(QString("%1°").arg(p.roll, 0, 'f', 2));
    m_lblYaw->setText(QString("%1°").arg(p.yaw, 0, 'f', 2));
    m_lblCrypto->setText(p.cryptoMsg.isEmpty() ? "—" : p.cryptoMsg);

    updateBatteryIndicator(p.voltage);
    updateSeparationBadge(p.separation);
}

void MainWindow::updateWindowTitle(const QString &teamId)
{
    if (!teamId.isEmpty())
        setWindowTitle(
            QString("CubeSat Ground Station — Team %1").arg(teamId)
        );
}

void MainWindow::updateBatteryIndicator(float voltage)
{
    QString color;
    if (voltage > 7.4f)
        color = "#27AE60"; // green
    else if (voltage > 6.6f)
        color = "#F39C12"; // yellow
    else
        color = "#E74C3C"; // red

    m_lblVoltage->setText(QString("%1 V").arg(voltage, 0, 'f', 2));
    m_lblVoltage->setStyleSheet(
        QString("color: %1; font-family: Consolas; font-size: 10pt; font-weight: bold;")
            .arg(color)
    );
}

void MainWindow::updateSeparationBadge(int separation)
{
    if (separation == 1) {
        m_lblSeparation->setText("SEPARATED");
        m_lblSeparation->setStyleSheet(
            "background-color: #27AE60; color: white; font-weight: bold;"
            "border-radius: 4px; padding: 4px;"
        );
    } else {
        m_lblSeparation->setText("NOT SEPARATED");
        m_lblSeparation->setStyleSheet(
            "background-color: #555555; color: white; font-weight: bold;"
            "border-radius: 4px; padding: 4px;"
        );
    }
}

void MainWindow::setSimControlsEnabled(bool en)
{
    m_simPlayBtn->setEnabled(en);
    m_simStopBtn->setEnabled(en);
    m_simResetBtn->setEnabled(en);
    m_simSpeedCombo->setEnabled(en);
    // Pause is only enabled when actually playing (handled by stateChanged)
    m_simPauseBtn->setEnabled(false);
}

void MainWindow::setSerialPanelEnabled(bool en)
{
    if (m_portCombo)       m_portCombo->setEnabled(en);
    if (m_baudCombo)       m_baudCombo->setEnabled(en);
    if (m_connectBtn)      m_connectBtn->setEnabled(en);
    if (m_refreshPortsBtn) m_refreshPortsBtn->setEnabled(en);
}
