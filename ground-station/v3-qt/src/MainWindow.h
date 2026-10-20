#pragma once

#include "TelemetryPacket.h"

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QPushButton;
class QTextEdit;
class QPlainTextEdit;
class QTabWidget;
class QDockWidget;
class QGroupBox;
class QProgressBar;
class QLineEdit;
class QSpinBox;
class QTimer;
QT_END_NAMESPACE

class SerialManager;
class CsvLogger;
class SimulationManager;
class ChartManager;
class MapView;
class CameraPanel;

/**
 * MainWindow
 * ----------
 * Top-level application window.  Wires all managers together and builds
 * the full UI: tabbed centre area + persistent right-side dock panel.
 *
 * Signal routing:
 *   Live mode:   SerialManager::packetReceived  → onPacketReceived()
 *   Sim mode:    SimulationManager::packetReceived → onPacketReceived()
 *   Never both simultaneously.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // ── Serial ───────────────────────────────────────────────────────────────
    void onConnectClicked();
    void onPortsUpdated(const QStringList &ports);
    void onSerialStatusChanged(const QString &status, bool connected);
    void onRawLineReceived(const QString &line);
    void onParseError(const QString &error);
    void onRefreshPortsClicked();

    // ── Commands ─────────────────────────────────────────────────────────────
    void onSendCryptoWord();
    void onTriggerSeparation();

    // ── Data ─────────────────────────────────────────────────────────────────
    void onPacketReceived(const TelemetryPacket &packet);
    void onLogStatusChanged(const QString &filePath, int rowCount);

    // ── Simulation ───────────────────────────────────────────────────────────
    void onSimBrowseClicked();
    void onSimPlayClicked();
    void onSimPauseClicked();
    void onSimStopClicked();
    void onSimResetClicked();
    void onSimSpeedChanged(int index);
    void onSimFileLoaded(int total, int skipped, int durationSec);
    void onSimFileLoadError(const QString &msg);
    void onSimProgressChanged(int current, int total);
    void onSimComplete();
    void onSimStateChanged(int state);   // int cast of SimulationManager::State

    // ── Port auto-refresh ─────────────────────────────────────────────────────
    void onPortRefreshTick();

private:
    // ── UI builders ──────────────────────────────────────────────────────────
    void buildCentralTabs();
    void buildSerialPanel(QWidget *parent);
    void buildRightDock();
    void buildDashboardPanel(QWidget *parent);
    void buildCommandPanel(QWidget *parent);
    void buildSimulationTab(QWidget *parent);
    void buildStatusBar();

    // ── Helpers ───────────────────────────────────────────────────────────────
    void setLiveMode(bool live);        ///< Switch between live/sim signal routing
    void appendRawLog(const QString &text, const QString &color = "#AAAAAA");
    void appendCommandLog(const QString &text);
    void updateDashboard(const TelemetryPacket &packet);
    void updateWindowTitle(const QString &teamId);
    void updateBatteryIndicator(float voltage);
    void updateSeparationBadge(int separation);
    void setSimControlsEnabled(bool en);
    void setSerialPanelEnabled(bool en);

    // ── Managers ─────────────────────────────────────────────────────────────
    SerialManager     *m_serial{nullptr};
    CsvLogger         *m_csvLogger{nullptr};
    SimulationManager *m_simMgr{nullptr};
    ChartManager      *m_chartMgr{nullptr};
    MapView           *m_mapView{nullptr};
    CameraPanel       *m_cameraPanel{nullptr};

    // ── Mode flag ─────────────────────────────────────────────────────────────
    bool m_simulationMode{false};

    // ── Serial panel widgets ──────────────────────────────────────────────────
    QComboBox   *m_portCombo{nullptr};
    QComboBox   *m_baudCombo{nullptr};
    QPushButton *m_connectBtn{nullptr};
    QPushButton *m_refreshPortsBtn{nullptr};
    QLabel      *m_connStatusLight{nullptr};
    QPlainTextEdit *m_rawLog{nullptr};
    QTimer      *m_portRefreshTimer{nullptr};

    // ── Dashboard widgets ─────────────────────────────────────────────────────
    QLabel *m_lblTeamId{nullptr};
    QLabel *m_lblPacketNo{nullptr};
    QLabel *m_lblTime{nullptr};
    QLabel *m_lblPressure{nullptr};
    QLabel *m_lblRelAlt{nullptr};
    QLabel *m_lblDescentSpeed{nullptr};
    QLabel *m_lblVoltage{nullptr};
    QLabel *m_lblGpsLat{nullptr};
    QLabel *m_lblGpsLng{nullptr};
    QLabel *m_lblGpsAlt{nullptr};
    QLabel *m_lblPitch{nullptr};
    QLabel *m_lblRoll{nullptr};
    QLabel *m_lblYaw{nullptr};
    QLabel *m_lblSeparation{nullptr};
    QLabel *m_lblCrypto{nullptr};

    // ── Command panel widgets ─────────────────────────────────────────────────
    QLineEdit   *m_cryptoInput{nullptr};
    QPushButton *m_sendCryptoBtn{nullptr};
    QPushButton *m_triggerSepBtn{nullptr};
    QTextEdit   *m_cmdLog{nullptr};

    // ── Simulation tab widgets ────────────────────────────────────────────────
    QLabel       *m_simFileLabel{nullptr};
    QPushButton  *m_simBrowseBtn{nullptr};
    QLabel       *m_simInfoLabel{nullptr};
    QPushButton  *m_simPlayBtn{nullptr};
    QPushButton  *m_simPauseBtn{nullptr};
    QPushButton  *m_simStopBtn{nullptr};
    QPushButton  *m_simResetBtn{nullptr};
    QComboBox    *m_simSpeedCombo{nullptr};
    QProgressBar *m_simProgress{nullptr};
    QLabel       *m_simElapsedLabel{nullptr};
    QLabel       *m_simPacketLabel{nullptr};

    // ── Status bar labels ─────────────────────────────────────────────────────
    QLabel *m_sbStatus{nullptr};
    QLabel *m_sbPackets{nullptr};
    QLabel *m_sbLastTs{nullptr};
    QLabel *m_sbCsvPath{nullptr};

    // ── Counters ──────────────────────────────────────────────────────────────
    int m_packetCount{0};

    // ── Port auto-refresh interval ────────────────────────────────────────────
    static constexpr int PORT_REFRESH_INTERVAL_MS = 10000;
};
