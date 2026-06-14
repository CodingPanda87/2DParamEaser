// src/app/MainWindow.h
#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "preview/ParamEaserPreview.h"
#include "core/ProjectManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();
    void onMarkerClicked(const std::string& dimId);
    void onExportSvg();
    void onDrawingSelected(QListWidgetItem* item);
    void onSaveProject();
    void onLoadProject();

private:
    void setupUi();
    void setupMenuBar();
    void updateStatusBar();
    void refreshAll();
    void loadDxfFile(const QString& path);

    // UI components
    QSplitter* m_splitter = nullptr;
    SvgPreviewWidget* m_leftPreview = nullptr;   // Left: DXF + markers
    SvgPreviewWidget* m_rightPreview = nullptr;  // Right: output SVG
    QListWidget* m_drawingList = nullptr;        // Drawing tabs (multi-draw)
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_exportBtn = nullptr;

    // Data
    ProjectManager m_projectManager;
    std::string m_currentDrawingKey;
};
