#include "MapView.h"

#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QFont>
#include <QFontMetrics>
#include <cmath>

static constexpr double PAD_FRACTION = 0.15; // 15% padding around trail bounds

MapView::MapView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    // Dark background via palette so it respects the QSS theme
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#1A1A1A"));
    setAutoFillBackground(true);
    setPalette(pal);
}

// ── Public slots ──────────────────────────────────────────────────────────────

void MapView::updatePosition(const TelemetryPacket &packet)
{
    // Ignore (0,0) — GPS not locked yet
    if (packet.gpsLat == 0.0 && packet.gpsLng == 0.0) {
        update();
        return;
    }

    m_lastLat = packet.gpsLat;
    m_lastLng = packet.gpsLng;
    m_lastAlt = packet.gpsAlt;

    m_trail.append({ packet.gpsLat, packet.gpsLng, packet.gpsAlt });
    m_hasData = true;

    recalcBounds();
    update(); // trigger repaint
}

void MapView::reset()
{
    m_trail.clear();
    m_hasData      = false;
    m_lastLat      = 0.0;
    m_lastLng      = 0.0;
    m_lastAlt      = 0.0;
    m_panOffsetX   = 0.0;
    m_panOffsetY   = 0.0;
    m_zoom         = 1.0;
    update();
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void MapView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect();

    // ── Background ────────────────────────────────────────────────────────────
    p.fillRect(r, QColor("#1A1A1A"));

    if (!m_hasData || m_trail.isEmpty()) {
        // No data yet — show waiting message
        p.setPen(QColor("#555555"));
        p.setFont(QFont("Segoe UI", 12));
        p.drawText(r, Qt::AlignCenter, "Waiting for GPS data…\n\nGPS fix required");
        drawOverlay(p);
        return;
    }

    // ── Grid lines ────────────────────────────────────────────────────────────
    p.setPen(QPen(QColor("#2A2A2A"), 1));
    const int gridLines = 6;
    for (int i = 1; i < gridLines; ++i) {
        int x = r.width()  * i / gridLines;
        int y = r.height() * i / gridLines;
        p.drawLine(x, 0, x, r.height());
        p.drawLine(0, y, r.width(), y);
    }

    // ── Trail polyline ────────────────────────────────────────────────────────
    if (m_trail.size() > 1) {
        QPainterPath path;
        QPointF first = geoToPixel(m_trail[0].lat, m_trail[0].lng);
        path.moveTo(first);

        for (int i = 1; i < m_trail.size(); ++i) {
            path.lineTo(geoToPixel(m_trail[i].lat, m_trail[i].lng));
        }

        // Shadow
        p.setPen(QPen(QColor(0, 0, 0, 80), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);

        // Trail
        p.setPen(QPen(QColor("#0078D4"), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);
    }

    // ── Waypoint dots (every 10th point) ─────────────────────────────────────
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#005FA3"));
    for (int i = 0; i < m_trail.size() - 1; i += 10) {
        QPointF pt = geoToPixel(m_trail[i].lat, m_trail[i].lng);
        p.drawEllipse(pt, 3.0, 3.0);
    }

    // ── Launch marker (first point) ───────────────────────────────────────────
    if (!m_trail.isEmpty()) {
        QPointF launch = geoToPixel(m_trail.first().lat, m_trail.first().lng);
        p.setBrush(QColor("#27AE60"));
        p.setPen(QPen(Qt::white, 1.5));
        p.drawEllipse(launch, 7.0, 7.0);
        p.setPen(QColor("#27AE60"));
        p.setFont(QFont("Segoe UI", 8));
        p.drawText(launch + QPointF(10, 4), "LAUNCH");
    }

    // ── Current position marker ───────────────────────────────────────────────
    QPointF cur = geoToPixel(m_lastLat, m_lastLng);

    // Outer ring
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(231, 76, 60, 100), 8));
    p.drawEllipse(cur, 14.0, 14.0);

    // Inner dot
    p.setBrush(QColor("#E74C3C"));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(cur, 7.0, 7.0);

    // ── Coordinate overlay ────────────────────────────────────────────────────
    drawOverlay(p);
}

void MapView::drawOverlay(QPainter &p)
{
    const QRect r = rect();

    // Bottom-left info box
    QString info;
    if (m_hasData) {
        info = QString("Lat: %1°   Lng: %2°   Alt: %3 m   Points: %4")
                   .arg(m_lastLat, 0, 'f', 6)
                   .arg(m_lastLng, 0, 'f', 6)
                   .arg(m_lastAlt, 0, 'f', 1)
                   .arg(m_trail.size());
    } else {
        info = "No GPS fix";
    }

    QFont font("Segoe UI", 9);
    QFontMetrics fm(font);
    int tw = fm.horizontalAdvance(info) + 16;
    int th = fm.height() + 10;

    QRect box(8, r.height() - th - 8, tw, th);
    p.fillRect(box, QColor(0, 0, 0, 170));
    p.setPen(QColor("#CCCCCC"));
    p.setFont(font);
    p.drawText(box, Qt::AlignCenter, info);

    // Top-right zoom/pan hint
    p.setPen(QColor("#444444"));
    p.setFont(QFont("Segoe UI", 8));
    p.drawText(r.adjusted(0, 6, -8, 0),
               Qt::AlignTop | Qt::AlignRight,
               "Scroll = zoom   Drag = pan");
}

// ── Interaction ───────────────────────────────────────────────────────────────

void MapView::wheelEvent(QWheelEvent *event)
{
    const double delta = event->angleDelta().y() / 120.0;
    m_zoom = qBound(0.2, m_zoom * std::pow(1.2, delta), 50.0);
    update();
}

void MapView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_lastMousePos = event->position();
}

void MapView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = event->position() - m_lastMousePos;
        m_lastMousePos = event->position();

        // Pan: convert pixel delta back to geo-space offset
        if (m_hasData && (m_maxLat - m_minLat) > 0 && (m_maxLng - m_minLng) > 0) {
            const double latRange = (m_maxLat - m_minLat) / m_zoom;
            const double lngRange = (m_maxLng - m_minLng) / m_zoom;
            m_panOffsetX -= delta.x() / width()  * lngRange;
            m_panOffsetY += delta.y() / height() * latRange;
        }
        update();
    }
}

void MapView::resizeEvent(QResizeEvent *)
{
    update();
}

// ── Private helpers ───────────────────────────────────────────────────────────

QPointF MapView::geoToPixel(double lat, double lng) const
{
    if (!m_hasData) return QPointF(width() / 2.0, height() / 2.0);

    const double latRange = (m_maxLat - m_minLat) / m_zoom;
    const double lngRange = (m_maxLng - m_minLng) / m_zoom;

    // Centre of the visible region (with pan offset)
    const double centreLat = (m_minLat + m_maxLat) / 2.0 + m_panOffsetY;
    const double centreLng = (m_minLng + m_maxLng) / 2.0 + m_panOffsetX;

    // Normalise to [0,1]
    const double nx = (lng - (centreLng - lngRange / 2.0)) / lngRange;
    // Latitude: top = maxLat, so invert Y
    const double ny = 1.0 - (lat - (centreLat - latRange / 2.0)) / latRange;

    return QPointF(nx * width(), ny * height());
}

void MapView::recalcBounds()
{
    if (m_trail.isEmpty()) return;

    m_minLat = m_maxLat = m_trail[0].lat;
    m_minLng = m_maxLng = m_trail[0].lng;

    for (const auto &pt : m_trail) {
        m_minLat = qMin(m_minLat, pt.lat);
        m_maxLat = qMax(m_maxLat, pt.lat);
        m_minLng = qMin(m_minLng, pt.lng);
        m_maxLng = qMax(m_maxLng, pt.lng);
    }

    // Prevent zero-range when only one point exists
    if (m_maxLat - m_minLat < 1e-6) { m_minLat -= 0.0005; m_maxLat += 0.0005; }
    if (m_maxLng - m_minLng < 1e-6) { m_minLng -= 0.0005; m_maxLng += 0.0005; }

    // Add padding
    const double latPad = (m_maxLat - m_minLat) * PAD_FRACTION;
    const double lngPad = (m_maxLng - m_minLng) * PAD_FRACTION;
    m_minLat -= latPad; m_maxLat += latPad;
    m_minLng -= lngPad; m_maxLng += lngPad;
}
