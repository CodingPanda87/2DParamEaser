// src/app/MainWindow.cpp
#include "app/MainWindow.h"
#include "app/ParamEditDialog.h"
#include "core/DxfParser.h"
#include "core/DxfToSvgRenderer.h"
#include "core/SvgGenerator.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>

// ── Conversion helper ──────────────────────────────────────────────
namespace {
MarkerInfo dimMarkerToInfo(const DimensionMarker& dm) {
    MarkerInfo mi;
    mi.id       = dm.id;
    mi.centerX  = dm.centerX;
    mi.centerY  = dm.centerY;
    mi.isBound  = dm.isBound;
    mi.label    = dm.label;
    mi.paramType = dm.paramType;
    return mi;
}
} // anonymous namespace

// ══════════════════════════════════════════════════════════════════
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("ParamEaser");
    resize(1200, 800);
    setupUi();
    setupMenuBar();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Drawing toolbar (multi-drawing support) ──
    auto* toolbar = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);

    m_drawingList = new QListWidget(this);
    m_drawingList->setMaximumHeight(36);
    m_drawingList->setFlow(QListView::LeftToRight);
    m_drawingList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_drawingList->setStyleSheet(
        "QListWidget{background:#16213e;border:none;border-bottom:1px solid rgba(255,255,255,0.06);}"
        "QListWidget::item{color:rgba(255,255,255,0.4);padding:4px 16px;margin:2px;"
        "border-radius:4px;font-size:12px;}"
        "QListWidget::item:selected{color:#4fc3f7;background:rgba(79,195,247,0.1);}"
    );
    connect(m_drawingList, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current) onDrawingSelected(current);
        });

    auto* addDrawingBtn = new QPushButton("+", this);
    addDrawingBtn->setFixedSize(28, 28);
    addDrawingBtn->setToolTip("添加图纸 (打开 DXF)");
    connect(addDrawingBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);

    toolbarLayout->addWidget(m_drawingList, 1);
    toolbarLayout->addWidget(addDrawingBtn);
    mainLayout->addWidget(toolbar);

    // ── Split pane: left = DXF with markers, right = output preview ──
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(3);

    m_leftPreview = new SvgPreviewWidget(this);
    m_rightPreview = new SvgPreviewWidget(this);

    m_splitter->addWidget(m_leftPreview);
    m_splitter->addWidget(m_rightPreview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_splitter, 1);

    // ── Status bar ──
    auto* statusBar = new QWidget(this);
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(12, 4, 12, 4);

    m_statusLabel = new QLabel("就绪", this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(120);
    m_progressBar->setMaximumHeight(6);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar{background:rgba(255,255,255,0.06);border:none;border-radius:3px;}"
        "QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #4fc3f7,stop:1 #7c4dff);border-radius:3px;}");

    m_exportBtn = new QPushButton("导出 SVG", this);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExportSvg);

    statusLayout->addWidget(m_statusLabel, 1);
    statusLayout->addWidget(m_progressBar);
    statusLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(statusBar);
    setCentralWidget(central);

    // ── Dark theme stylesheet ──
    setStyleSheet(
        "QMainWindow{background:#1a1a2e;}"
        "QSplitter::handle{background:rgba(79,195,247,0.2);}"
        "QSplitter::handle:hover{background:rgba(79,195,247,0.4);}"
        "QLabel{color:rgba(255,255,255,0.5);font-size:12px;}"
    );

    // Wire marker click callback
    m_leftPreview->setMarkerClickCallback(
        [this](const std::string& id) { onMarkerClicked(id); });
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction("打开项目...", this, &MainWindow::onLoadProject, QKeySequence("Ctrl+Shift+O"));
    fileMenu->addAction("保存项目...", this, &MainWindow::onSaveProject, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("打开 DXF...", this, &MainWindow::onOpenFile, QKeySequence::Open);
    fileMenu->addAction("导出 SVG...", this, &MainWindow::onExportSvg, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close, QKeySequence::Quit);
}

void MainWindow::onOpenFile() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "打开 DXF 文件", {},
                                                        "DXF Files (*.dxf)");
    for (const auto& path : paths)
        loadDxfFile(path);
}

void MainWindow::loadDxfFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + path);
        return;
    }

    QString content = file.readAll();
    std::string dxf = content.toStdString();
    QFileInfo fi(path);

    // Generate unique key from filename
    std::string key = fi.baseName().toStdString();
    if (key.empty()) key = "drawing";

    // Check for duplicate
    if (m_projectManager.hasDrawing(key)) {
        int suffix = 2;
        while (m_projectManager.hasDrawing(key + "_" + std::to_string(suffix)))
            suffix++;
        key = key + "_" + std::to_string(suffix);
    }

    try {
        DxfParser parser;
        auto dims = parser.parseDimensions(dxf);
        if (parser.hasError()) {
            QMessageBox::warning(this, "解析错误",
                QString::fromStdString(parser.lastError()));
        }

        if (dims.empty()) {
            QMessageBox::information(this, "提示",
                "文件中未检测到尺寸标注。\n\n仅显示图纸预览。");
        }

        DxfToSvgRenderer renderer;
        std::string svg = renderer.render(dxf);

        DrawingData dd;
        dd.key = key;
        dd.sourceFile = path.toStdString();
        dd.rawDxfContent = dxf;
        dd.drawingSvg = svg;
        dd.dimensions = dims;

        m_projectManager.addDrawing(key, std::move(dd));

        auto* item = new QListWidgetItem(QString::fromStdString(key));
        item->setData(Qt::UserRole, QString::fromStdString(key));
        m_drawingList->addItem(item);
        m_drawingList->setCurrentItem(item);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "解析错误",
            "无法解析 DXF 文件:\n" + QString::fromStdString(e.what()));
        return;
    }
}

void MainWindow::onDrawingSelected(QListWidgetItem* item) {
    if (!item) return;
    m_currentDrawingKey = item->data(Qt::UserRole).toString().toStdString();
    refreshAll();
}

void MainWindow::refreshAll() {
    if (m_currentDrawingKey.empty()) return;

    auto* dd = m_projectManager.getDrawing(m_currentDrawingKey);
    if (!dd) return;

    // Left panel: DXF + markers
    auto markers = m_projectManager.getMarkers(m_currentDrawingKey);
    std::vector<MarkerInfo> converted;
    converted.reserve(markers.size());
    for (const auto& m : markers)
        converted.push_back(dimMarkerToInfo(m));
    m_leftPreview->setContent(QString::fromStdString(dd->drawingSvg), converted);

    // Right panel: output SVG
    SvgGenerator gen;
    std::string outputSvg = gen.generateWithMetadata(
        dd->drawingSvg, dd->dimensions, dd->sourceFile);
    m_rightPreview->setContent(QString::fromStdString(outputSvg), {});

    updateStatusBar();
}

void MainWindow::onMarkerClicked(const std::string& dimId) {
    if (m_currentDrawingKey.empty()) return;

    auto* dim = m_projectManager.findDimension(m_currentDrawingKey, dimId);
    if (!dim) return;

    ParamEditDialog dialog(dim, this);
    if (dialog.exec() == QDialog::Accepted) {
        auto result = dialog.result();
        if (result.bind) {
            m_projectManager.bindParam(m_currentDrawingKey, dimId,
                                        result.type, result.key, result.defaultValue);
        } else {
            m_projectManager.unbindParam(m_currentDrawingKey, dimId);
        }
        refreshAll();
    }
}

void MainWindow::onExportSvg() {
    if (m_currentDrawingKey.empty()) return;

    auto* dd = m_projectManager.getDrawing(m_currentDrawingKey);
    if (!dd) return;

    QString path = QFileDialog::getSaveFileName(this, "导出 SVG",
        QString::fromStdString(dd->key) + ".svg", "SVG Files (*.svg)");
    if (path.isEmpty()) return;

    SvgGenerator gen;
    std::string svg = gen.generateWithMetadata(dd->drawingSvg, dd->dimensions, dd->sourceFile);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(svg.data(), svg.size());
        file.close();
        m_statusLabel->setText("已导出: " + path);
    } else {
        QMessageBox::warning(this, "错误", "无法写入文件");
    }
}

void MainWindow::updateStatusBar() {
    if (m_currentDrawingKey.empty()) return;

    int total = m_projectManager.totalCount(m_currentDrawingKey);
    int bound = m_projectManager.boundCount(m_currentDrawingKey);
    int unbound = m_projectManager.unboundCount(m_currentDrawingKey);

    m_statusLabel->setText(
        QString("📐 尺寸: %1  ✅ 已绑定: %2  ⚠️ 未绑定: %3")
            .arg(total).arg(bound).arg(unbound));

    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(bound);
    } else {
        m_progressBar->setMaximum(0);
        m_progressBar->setValue(0);
    }

    m_exportBtn->setEnabled(total > 0);
}

void MainWindow::onSaveProject() {
    if (m_currentDrawingKey.empty()) return;

    QString path = QFileDialog::getSaveFileName(this, "保存项目",
        "project.parameaser", "ParamEaser Project (*.parameaser)");
    if (path.isEmpty()) return;

    // Save all drawings, not just current
    std::string json = m_projectManager.toJson();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(json.data(), static_cast<qint64>(json.size()));
        file.close();
        m_statusLabel->setText("项目已保存");
    } else {
        QMessageBox::warning(this, "错误", "无法保存项目文件");
    }
}

void MainWindow::onLoadProject() {
    QString path = QFileDialog::getOpenFileName(this, "打开项目", {},
        "ParamEaser Project (*.parameaser)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开项目文件");
        return;
    }

    std::string json = file.readAll().toStdString();

    // Simple approach: load project and refresh UI
    // ProjectManager::fromJson needs to create drawings with SVG
    // For now, load the JSON metadata
    m_projectManager = ProjectManager::fromJson(json);

    // Rebuild drawing list
    m_drawingList->clear();
    for (const auto& key : m_projectManager.drawingKeys()) {
        auto* item = new QListWidgetItem(QString::fromStdString(key));
        item->setData(Qt::UserRole, QString::fromStdString(key));
        m_drawingList->addItem(item);
    }

    if (!m_projectManager.drawingKeys().empty()) {
        m_drawingList->setCurrentRow(0);
    }

    m_statusLabel->setText("项目已加载: " + QFileInfo(path).fileName());
}
