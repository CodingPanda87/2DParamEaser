// src/preview/ParamEaserPreview.h
// ════════════════════════════════════════════════════════════════
//  Single-header reusable SVG preview widget for ParamEaser.
//  Any external system can integrate by including this one file:
//
//     #include "ParamEaserPreview.h"
//
//  Features:
//   - Display SVG with zoom/pan
//   - Interactive dimension markers overlay
//   - Click/double-click signals for marker interaction
//   - Zoom controls (scroll wheel, fit-to-view)
//   - Pimpl implementation (no internal details leaked)
// ════════════════════════════════════════════════════════════════
#pragma once

#include <QWidget>
#include <QString>
#include <memory>
#include <vector>
#include <functional>

// ── Lightweight marker data for external systems ──
struct MarkerInfo {
    std::string id;
    double centerX = 0.0;
    double centerY = 0.0;
    bool isBound = false;
    std::string label;      // param key if bound, dim id if not
    std::string paramType;  // "float", "int", or empty
};

// ── Callback types ──
using MarkerClickCallback = std::function<void(const std::string& markerId)>;
using MarkerDoubleClickCallback = std::function<void(const std::string& markerId)>;

// ── SvgPreviewWidget ──
//  A reusable QWidget that renders an SVG with interactive markers.
//  Drop this widget into any Qt application, call setContent(),
//  and connect the callbacks.
class SvgPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit SvgPreviewWidget(QWidget* parent = nullptr);
    ~SvgPreviewWidget() override;

    // Set SVG content and markers to display
    void setContent(const QString& svgContent,
                    const std::vector<MarkerInfo>& markers);

    // Clear the display
    void clear();

    // Zoom control
    void setZoom(double factor);       // 1.0 = 100%
    void fitToView();                  // Fit SVG to widget size
    double zoom() const;

    // Callbacks (C++ friendly, no moc dependency for callers)
    void setMarkerClickCallback(MarkerClickCallback cb);
    void setMarkerDoubleClickCallback(MarkerDoubleClickCallback cb);

    // Access current markers (for external state sync)
    const std::vector<MarkerInfo>& markers() const;

signals:
    // Qt signals (for moc-based integration)
    void markerClicked(const QString& markerId);
    void markerDoubleClicked(const QString& markerId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    class Impl;
    std::unique_ptr<Impl> d;
};
