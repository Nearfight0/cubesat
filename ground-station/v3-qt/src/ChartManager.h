#pragma once

#include "TelemetryPacket.h"

#include <QObject>
#include <QWidget>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAbstractAxis>

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_NAMESPACE

class ChartManager : public QObject
{
    Q_OBJECT

public:
    explicit ChartManager(QObject *parent = nullptr);
    ~ChartManager() override = default;

    QWidget *widget() const { return m_container; }

public slots:
    void updateCharts(const TelemetryPacket &packet);
    void reset();

private:
    // Per-chart bundle: chart + view + cached axis pointers + running y-bounds
    struct ChartBundle {
        QChart          *chart{nullptr};
        QChartView      *view{nullptr};
        QAbstractAxis   *xAxis{nullptr};
        QAbstractAxis   *yAxis{nullptr};
        double           yMin{0.0};
        double           yMax{1.0};
    };

    ChartBundle makeChart(const QString &title);
    void        applyDarkTheme(QChart *chart);

    // Append one point and scroll X; returns the elapsed-seconds value used
    double appendPoint(QLineSeries *series, ChartBundle &bundle,
                       qint64 msEpoch, double value);

    // Recompute yMin/yMax for a bundle from visible points in [xMin, xMax]
    void   rescaleY(ChartBundle &bundle, double xMin, double xMax);

    QWidget     *m_container{nullptr};
    QGridLayout *m_grid{nullptr};

    ChartBundle m_altBundle;
    ChartBundle m_speedBundle;
    ChartBundle m_voltBundle;
    ChartBundle m_pressBundle;
    ChartBundle m_imuBundle;

    QLineSeries *m_altSeries{nullptr};
    QLineSeries *m_speedSeries{nullptr};
    QLineSeries *m_voltSeries{nullptr};
    QLineSeries *m_pressSeries{nullptr};
    QLineSeries *m_pitchSeries{nullptr};
    QLineSeries *m_rollSeries{nullptr};
    QLineSeries *m_yawSeries{nullptr};

    qint64 m_firstMs{0};
    bool   m_hasData{false};

    // Rescale Y only every N packets to avoid O(n) scan every second
    int m_packetCount{0};

    static constexpr int    WINDOW_SEC  = 60;
    static constexpr int    MAX_POINTS  = 500;
    static constexpr int    RESCALE_EVERY = 5; // full Y-rescan every 5 packets
};
