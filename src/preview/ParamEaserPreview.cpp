// src/preview/ParamEaserPreview.cpp
#include "preview/ParamEaserPreview.h"
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QToolTip>
#include <cmath>

class SvgPreviewWidget::Impl {
public:
    QSvgRenderer* renderer = nullptr;
    std::vector<MarkerInfo> markers;
    QString svgContent;

    // View transform
    double zoomFactor = 1.0;
    double panX = 0.0, panY = 0.0;

    // Interaction state
    bool dragging = false;
    double lastMouseX = 0, lastMouseY = 0;
    int hoveredMarker = -1;

    // Callbacks
    MarkerClickCallback clickCb;
    MarkerDoubleClickCallback doubleClickCb;

    QSizeF svgSize;

    ~Impl() { delete renderer; }

    void loadSvg(const QString& svg) {
        delete renderer;
        renderer = new QSvgRenderer();
        svgContent = svg;
        if (!renderer->load(svg.toUtf8())) {
            delete renderer;
            renderer = nullptr;
            return;
        }
        QRectF vb = renderer->viewBoxF();
        svgSize = renderer->defaultSize();
        if (svgSize.isEmpty() || svgSize.isNull())
            svgSize = vb.size();
        if (svgSize.isEmpty() || svgSize.isNull())
            svgSize = QSizeF(100, 100);
        zoomFactor = 1.0;
        panX = 0; panY = 0;
    }

    // Transform marker SVG-coordinates to widget pixel-coordinates
    QPointF markerToWidget(const MarkerInfo& m, const QRectF& widgetRect) const {
        double scale = std::min(widgetRect.width() / svgSize.width(),
                                 widgetRect.height() / svgSize.height()) * zoomFactor;
        double cx = widgetRect.center().x() + (m.centerX - svgSize.width() / 2) * scale + panX;
        double cy = widgetRect.center().y() + (m.centerY - svgSize.height() / 2) * scale + panY;
        return {cx, cy};
    }

    // Find marker index under widget coordinate (hit-test radius = 12px)
    int hitTest(const QPointF& wpos, const QRectF& widgetRect) const {
        for (int i = 0; i < static_cast<int>(markers.size()); ++i) {
            QPointF mp = markerToWidget(markers[i], widgetRect);
            double dx = wpos.x() - mp.x();
            double dy = wpos.y() - mp.y();
            if (std::sqrt(dx * dx + dy * dy) < 12.0) return i;
        }
        return -1;
    }
};

// ── Public API ──

SvgPreviewWidget::SvgPreviewWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Impl>()) {
    setMouseTracking(true);
    setMinimumSize(200, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::ArrowCursor);
}

SvgPreviewWidget::~SvgPreviewWidget() = default;

void SvgPreviewWidget::setContent(const QString& svgContent,
                                   const std::vector<MarkerInfo>& markers) {
    d->markers = markers;
    d->loadSvg(svgContent);
    fitToView();
    update();
}

void SvgPreviewWidget::clear() {
    d->markers.clear();
    delete d->renderer; d->renderer = nullptr;
    d->svgContent.clear();
    d->zoomFactor = 1.0;
    d->panX = d->panY = 0;
    update();
}

void SvgPreviewWidget::setZoom(double factor) {
    d->zoomFactor = std::max(0.1, std::min(10.0, factor));
    update();
}

void SvgPreviewWidget::fitToView() {
    if (!d->renderer) return;
    QRectF wr = rect();
    double scaleX = wr.width() / d->svgSize.width();
    double scaleY = wr.height() / d->svgSize.height();
    d->zoomFactor = std::min(scaleX, scaleY) * 0.85;
    d->panX = 0;
    d->panY = 0;
    update();
}

double SvgPreviewWidget::zoom() const { return d->zoomFactor; }

void SvgPreviewWidget::setMarkerClickCallback(MarkerClickCallback cb) {
    d->clickCb = std::move(cb);
}

void SvgPreviewWidget::setMarkerDoubleClickCallback(MarkerDoubleClickCallback cb) {
    d->doubleClickCb = std::move(cb);
}

const std::vector<MarkerInfo>& SvgPreviewWidget::markers() const {
    return d->markers;
}

// ── Event handlers ──

void SvgPreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor("#0f0f23"));

    // Grid dots
    QPen gridPen(QColor(255, 255, 255, 15));
    p.setPen(gridPen);
    for (int x = 0; x < width(); x += 20)
        for (int y = 0; y < height(); y += 20)
            p.drawPoint(x, y);

    if (!d->renderer) {
        // Empty state
        p.setPen(QColor(255, 255, 255, 30));
        p.setFont(QFont("sans-serif", 12));
        p.drawText(rect(), Qt::AlignCenter, "打开 DXF 文件开始");
        return;
    }

    // Render SVG
    QRectF wr = rect();
    double scale = std::min(wr.width() / d->svgSize.width(),
                             wr.height() / d->svgSize.height()) * d->zoomFactor;
    double drawW = d->svgSize.width() * scale;
    double drawH = d->svgSize.height() * scale;
    double drawX = wr.center().x() - drawW / 2 + d->panX;
    double drawY = wr.center().y() - drawH / 2 + d->panY;
    QRectF drawRect(drawX, drawY, drawW, drawH);

    if (d->renderer && d->renderer->isValid())
        d->renderer->render(&p, drawRect);

    // Draw markers
    for (int i = 0; i < static_cast<int>(d->markers.size()); ++i) {
        const auto& m = d->markers[i];
        QPointF mp = d->markerToWidget(m, wr);
        bool hovered = (i == d->hoveredMarker);

        // Outer ring (glow effect)
        QColor ringColor = m.isBound ? QColor("#4caf50") : QColor("#ff9800");
        ringColor.setAlpha(hovered ? 180 : 80);
        p.setPen(QPen(ringColor, hovered ? 3 : 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(mp, hovered ? 10 : 8, hovered ? 10 : 8);

        // Inner dot
        QColor dotColor = m.isBound ? QColor("#4caf50") : QColor("#ff9800");
        p.setPen(QPen(Qt::white, 1.5));
        p.setBrush(dotColor);
        p.drawEllipse(mp, 5, 5);

        // Label above marker
        p.setPen(m.isBound ? QColor("#4caf50") : QColor("#ff9800"));
        p.setFont(QFont("monospace", 8, QFont::Bold));
        QRectF labelRect(mp.x() - 40, mp.y() - 22, 80, 14);
        p.drawText(labelRect, Qt::AlignCenter, QString::fromStdString(m.label));
    }
}

void SvgPreviewWidget::resizeEvent(QResizeEvent*) {
    update();
}

void SvgPreviewWidget::mousePressEvent(QMouseEvent* e) {
    int hit = d->hitTest(QPointF(e->pos()), rect());
    if (hit >= 0) {
        const auto& id = d->markers[hit].id;
        if (d->clickCb) d->clickCb(id);
        emit markerClicked(QString::fromStdString(id));
    } else {
        d->dragging = true;
        d->lastMouseX = e->position().x();
        d->lastMouseY = e->position().y();
        setCursor(Qt::ClosedHandCursor);
    }
}

void SvgPreviewWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    int hit = d->hitTest(QPointF(e->pos()), rect());
    if (hit >= 0) {
        const auto& id = d->markers[hit].id;
        if (d->doubleClickCb) d->doubleClickCb(id);
        emit markerDoubleClicked(QString::fromStdString(id));
    }
}

void SvgPreviewWidget::mouseMoveEvent(QMouseEvent* e) {
    QPointF pos = e->position();
    if (d->dragging) {
        double dx = pos.x() - d->lastMouseX;
        double dy = pos.y() - d->lastMouseY;
        d->panX += dx;
        d->panY += dy;
        d->lastMouseX = pos.x();
        d->lastMouseY = pos.y();
        update();
        return;
    }

    int hit = d->hitTest(pos, rect());
    if (hit != d->hoveredMarker) {
        d->hoveredMarker = hit;
        if (hit >= 0) {
            setCursor(Qt::PointingHandCursor);
            const auto& m = d->markers[hit];
            QString tip = QString::fromStdString(m.id);
            if (m.isBound)
                tip += " (" + QString::fromStdString(m.paramType) + ")";
            else
                tip += " (未绑定)";
            setToolTip(tip);
        } else {
            setCursor(Qt::ArrowCursor);
            setToolTip("");
        }
        update();
    }
}

void SvgPreviewWidget::mouseReleaseEvent(QMouseEvent*) {
    d->dragging = false;
    setCursor(d->hoveredMarker >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void SvgPreviewWidget::wheelEvent(QWheelEvent* e) {
    double delta = e->angleDelta().y() / 120.0;
    double factor = (delta > 0) ? 1.15 : (1.0 / 1.15);

    // Zoom toward cursor position
    QRectF wr = rect();
    double mx = e->position().x() - wr.center().x() - d->panX;
    double my = e->position().y() - wr.center().y() - d->panY;

    double oldScale = d->zoomFactor;
    d->zoomFactor = std::max(0.1, std::min(10.0, d->zoomFactor * factor));
    double newScale = d->zoomFactor;

    // Adjust pan to keep cursor position stable
    d->panX += mx * (1.0 - newScale / oldScale);
    d->panY += my * (1.0 - newScale / oldScale);

    update();
}
