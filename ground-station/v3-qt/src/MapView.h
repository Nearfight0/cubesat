#pragma once

#include "TelemetryPacket.h"

#include <QWidget>
#include <QVector>
#include <QPointF>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

/**
 * MapView
 * -------
 * Lightweight GPS trail viewer drawn with QPainter.
 * No Qt Location / QML / OpenGL — zero startup lag, works fully offline.
 *
 * Displays:
 *  - All previous GPS positions as a connected trail (blue polyline)
 *  - Current position as an animated red dot
 *  - Lat / Lng / Alt overlay text
 *  - Auto-scales to fit all received coordinates
 */
class MapView : public QWidget
{
    Q_OBJECT

public:
    explicit MapView(QWidget *parent = nullptr);
    ~MapView() override = default;

public slots:
    void updatePosition(const TelemetryPacket &packet);
    void reset();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPointF geoToPixel(double lat, double lng) const;
    void    recalcBounds();
    void    drawOverlay(QPainter &p);

    struct GeoPoint {
        double lat{0.0};
        double lng{0.0};
        double alt{0.0};
    };

    QVector<GeoPoint> m_trail;

    // Visible bounding box (with padding)
    double m_minLat{0.0}, m_maxLat{0.0};
    double m_minLng{0.0}, m_maxLng{0.0};

    double m_lastLat{0.0};
    double m_lastLng{0.0};
    double m_lastAlt{0.0};

    // Pan support
    QPointF m_lastMousePos;
    double  m_panOffsetX{0.0};
    double  m_panOffsetY{0.0};

    // Zoom
    double m_zoom{1.0};

    bool m_hasData{false};
};
