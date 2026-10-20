#include "ChartManager.h"

#include <QGridLayout>
#include <QDateTime>
#include <QtCharts/QValueAxis>
#include <limits>

// ── Ctor ──────────────────────────────────────────────────────────────────────

ChartManager::ChartManager(QObject *parent)
    : QObject(parent)
{
    m_container = new QWidget;
    m_grid = new QGridLayout(m_container);
    m_grid->setSpacing(4);
    m_grid->setContentsMargins(4, 4, 4, 4);

    m_altBundle   = makeChart("Relative Altitude [m]");
    m_speedBundle = makeChart("Descent Speed [m/s]");
    m_voltBundle  = makeChart("Battery Voltage [V]");
    m_pressBundle = makeChart("Atmospheric Pressure [Pa]");
    m_imuBundle   = makeChart("Pitch / Roll / Yaw [deg]");

    // ── Series ────────────────────────────────────────────────────────────────
    auto makeSeries = [](const QString &name, const QString &hex) {
        auto *s = new QLineSeries;
        s->setName(name);
        s->setColor(QColor(hex));
        // Disable OpenGL per-series (avoids driver overhead at 1 Hz)
        s->setUseOpenGL(false);
        return s;
    };

    m_altSeries   = makeSeries("Altitude",      "#0078D4");
    m_speedSeries = makeSeries("Descent Speed", "#27AE60");
    m_voltSeries  = makeSeries("Voltage",       "#F39C12");
    m_pressSeries = makeSeries("Pressure",      "#9B59B6");
    m_pitchSeries = makeSeries("Pitch",         "#E74C3C");
    m_rollSeries  = makeSeries("Roll",          "#1ABC9C");
    m_yawSeries   = makeSeries("Yaw",           "#E67E22");

    m_altBundle.chart->addSeries(m_altSeries);
    m_speedBundle.chart->addSeries(m_speedSeries);
    m_voltBundle.chart->addSeries(m_voltSeries);
    m_pressBundle.chart->addSeries(m_pressSeries);
    m_imuBundle.chart->addSeries(m_pitchSeries);
    m_imuBundle.chart->addSeries(m_rollSeries);
    m_imuBundle.chart->addSeries(m_yawSeries);

    // ── Axes (create once, cache pointers) ────────────────────────────────────
    for (auto *bundle : {&m_altBundle, &m_speedBundle, &m_voltBundle,
                         &m_pressBundle, &m_imuBundle})
    {
        bundle->chart->createDefaultAxes();

        const auto hAxes = bundle->chart->axes(Qt::Horizontal);
        const auto vAxes = bundle->chart->axes(Qt::Vertical);
        bundle->xAxis = hAxes.isEmpty() ? nullptr : hAxes.first();
        bundle->yAxis = vAxes.isEmpty() ? nullptr : vAxes.first();

        if (bundle->xAxis) {
            bundle->xAxis->setRange(0, WINDOW_SEC);
            bundle->xAxis->setLabelsColor(QColor("#AAAAAA"));
            bundle->xAxis->setGridLineColor(QColor("#2E2E2E"));
            QFont f("Segoe UI", 8); bundle->xAxis->setLabelsFont(f);
        }
        if (bundle->yAxis) {
            bundle->yAxis->setRange(0, 1);
            bundle->yAxis->setLabelsColor(QColor("#AAAAAA"));
            bundle->yAxis->setGridLineColor(QColor("#2E2E2E"));
            QFont f("Segoe UI", 8); bundle->yAxis->setLabelsFont(f);
        }
    }

    // ── Grid layout ───────────────────────────────────────────────────────────
    m_grid->addWidget(m_altBundle.view,   0, 0);
    m_grid->addWidget(m_speedBundle.view, 0, 1);
    m_grid->addWidget(m_voltBundle.view,  0, 2);
    m_grid->addWidget(m_pressBundle.view, 1, 0);
    m_grid->addWidget(m_imuBundle.view,   1, 1, 1, 2);
    for (int c = 0; c < 3; ++c) m_grid->setColumnStretch(c, 1);
    for (int r = 0; r < 2; ++r) m_grid->setRowStretch(r, 1);
}

// ── Public slots ──────────────────────────────────────────────────────────────

void ChartManager::updateCharts(const TelemetryPacket &packet)
{
    const qint64 ms = packet.timestamp.isValid()
                      ? packet.timestamp.toMSecsSinceEpoch()
                      : QDateTime::currentMSecsSinceEpoch();

    if (!m_hasData) { m_firstMs = ms; m_hasData = true; }

    const double elapsed = (ms - m_firstMs) / 1000.0;
    const double xMin    = qMax(0.0, elapsed - WINDOW_SEC);
    const double xMax    = elapsed + 1.0;

    // Append all points (cheap — just list append + X-axis update)
    appendPoint(m_altSeries,   m_altBundle,   ms, packet.relAltitude);
    appendPoint(m_speedSeries, m_speedBundle, ms, packet.descentSpeed);
    appendPoint(m_voltSeries,  m_voltBundle,  ms, packet.voltage);
    appendPoint(m_pressSeries, m_pressBundle, ms, packet.pressure);
    appendPoint(m_pitchSeries, m_imuBundle,   ms, packet.pitch);
    appendPoint(m_rollSeries,  m_imuBundle,   ms, packet.roll);
    appendPoint(m_yawSeries,   m_imuBundle,   ms, packet.yaw);

    // Update X axes (cheap — just set two doubles)
    for (auto *b : {&m_altBundle, &m_speedBundle, &m_voltBundle,
                    &m_pressBundle, &m_imuBundle})
    {
        if (b->xAxis) b->xAxis->setRange(xMin, xMax);
    }

    // Full Y-rescan only every RESCALE_EVERY packets
    ++m_packetCount;
    if (m_packetCount % RESCALE_EVERY == 0) {
        rescaleY(m_altBundle,   xMin, xMax);
        rescaleY(m_speedBundle, xMin, xMax);
        rescaleY(m_voltBundle,  xMin, xMax);
        rescaleY(m_pressBundle, xMin, xMax);
        rescaleY(m_imuBundle,   xMin, xMax);
    } else {
        // Fast path: only expand Y range if new value exceeds current range
        auto quickExpand = [](ChartBundle &b, double v) {
            bool changed = false;
            if (v < b.yMin) { b.yMin = v - qAbs(v) * 0.05 - 0.5; changed = true; }
            if (v > b.yMax) { b.yMax = v + qAbs(v) * 0.05 + 0.5; changed = true; }
            if (changed && b.yAxis) b.yAxis->setRange(b.yMin, b.yMax);
        };
        quickExpand(m_altBundle,   packet.relAltitude);
        quickExpand(m_speedBundle, packet.descentSpeed);
        quickExpand(m_voltBundle,  packet.voltage);
        quickExpand(m_pressBundle, packet.pressure);
        quickExpand(m_imuBundle,   packet.pitch);
        quickExpand(m_imuBundle,   packet.roll);
        quickExpand(m_imuBundle,   packet.yaw);
    }
}

void ChartManager::reset()
{
    for (auto *s : {m_altSeries, m_speedSeries, m_voltSeries,
                    m_pressSeries, m_pitchSeries, m_rollSeries, m_yawSeries})
        s->clear();

    for (auto *b : {&m_altBundle, &m_speedBundle, &m_voltBundle,
                    &m_pressBundle, &m_imuBundle})
    {
        b->yMin = 0.0; b->yMax = 1.0;
        if (b->xAxis) b->xAxis->setRange(0, WINDOW_SEC);
        if (b->yAxis) b->yAxis->setRange(0, 1);
    }

    m_hasData     = false;
    m_firstMs     = 0;
    m_packetCount = 0;
}

// ── Private ───────────────────────────────────────────────────────────────────

ChartManager::ChartBundle ChartManager::makeChart(const QString &title)
{
    auto *chart = new QChart;
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::NoAnimation); // critical for 1 Hz updates
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setMargins(QMargins(4, 4, 4, 4));
    applyDarkTheme(chart);

    auto *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing, false); // faster at 1 Hz
    view->setMinimumSize(240, 180);

    return { chart, view, nullptr, nullptr, 0.0, 1.0 };
}

void ChartManager::applyDarkTheme(QChart *chart)
{
    chart->setTheme(QChart::ChartThemeDark);
    chart->setBackgroundBrush(QBrush(QColor("#1C1C1C")));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->setTitleFont(QFont("Segoe UI", 8, QFont::Bold));
    chart->legend()->setLabelColor(QColor("#CCCCCC"));
    chart->legend()->setFont(QFont("Segoe UI", 8));
}

double ChartManager::appendPoint(QLineSeries *series, ChartBundle &bundle,
                                  qint64 msEpoch, double value)
{
    const double elapsed = (msEpoch - m_firstMs) / 1000.0;
    series->append(elapsed, value);

    // Trim oldest point if over cap — only touch the list once
    if (series->count() > MAX_POINTS)
        series->remove(0);

    return elapsed;
}

void ChartManager::rescaleY(ChartBundle &bundle, double xMin, double xMax)
{
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();

    for (auto *s : bundle.chart->series()) {
        auto *ls = qobject_cast<QLineSeries *>(s);
        if (!ls) continue;
        for (const QPointF &p : ls->points()) {
            if (p.x() < xMin) continue;
            if (p.y() < yMin) yMin = p.y();
            if (p.y() > yMax) yMax = p.y();
        }
    }

    if (yMin > yMax) { yMin = 0.0; yMax = 1.0; }

    const double pad = (yMax - yMin) * 0.08 + 0.5;
    bundle.yMin = yMin - pad;
    bundle.yMax = yMax + pad;

    if (bundle.yAxis) bundle.yAxis->setRange(bundle.yMin, bundle.yMax);
}
