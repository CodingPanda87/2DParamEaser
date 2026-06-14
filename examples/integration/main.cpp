// examples/integration/main.cpp
// ════════════════════════════════════════════════════════════════
//  Example: External system using ParamEaserPreview single header
//
//  This shows how little code is needed to embed SVG preview
//  with interactive markers in any Qt application.
// ════════════════════════════════════════════════════════════════
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

// ★ The one include you need ★
#include "preview/ParamEaserPreview.h"

// For DXF parsing (or skip if system already has dimension data)
#include "core/DxfParser.h"
#include "core/DxfToSvgRenderer.h"

class IntegrationDemo : public QWidget {
    Q_OBJECT
public:
    IntegrationDemo() {
        auto* layout = new QVBoxLayout(this);

        // Create the preview widget
        m_preview = new SvgPreviewWidget(this);
        m_preview->setMinimumHeight(400);

        m_statusLabel = new QLabel("就绪", this);

        auto* btnLayout = new QHBoxLayout();
        auto* openBtn = new QPushButton("打开 DXF", this);
        btnLayout->addWidget(openBtn);
        btnLayout->addWidget(m_statusLabel);
        btnLayout->addStretch();

        layout->addWidget(m_preview);
        layout->addLayout(btnLayout);

        // Connect marker click callback
        m_preview->setMarkerClickCallback([this](const std::string& id) {
            m_statusLabel->setText(QString("点击标记: %1").arg(QString::fromStdString(id)));
        });

        connect(openBtn, &QPushButton::clicked, this, [this]() {
            QString path = QFileDialog::getOpenFileName(this, "打开 DXF", {},
                                                         "DXF Files (*.dxf)");
            if (path.isEmpty()) return;

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "错误", "无法打开文件");
                return;
            }

            QString content = file.readAll();
            std::string dxf = content.toStdString();

            // Parse dimensions
            DxfParser parser;
            auto dims = parser.parseDimensions(dxf);

            // Convert DXF to SVG
            DxfToSvgRenderer renderer;
            std::string svg = renderer.render(dxf);

            // Build markers
            std::vector<MarkerInfo> markers;
            for (size_t i = 0; i < dims.size(); ++i) {
                MarkerInfo m;
                m.id = dims[i].id;
                m.centerX = dims[i].centerX;
                m.centerY = dims[i].centerY;
                m.isBound = dims[i].isBound();
                m.label = dims[i].isBound() ? dims[i].param->key : dims[i].id;
                m.paramType = dims[i].isBound() ? dims[i].param->type : "";
                markers.push_back(m);
            }

            // Set content — one call, widget handles the rest
            m_preview->setContent(QString::fromStdString(svg), markers);

            m_statusLabel->setText(QString("已加载: %1 | %2 个尺寸")
                .arg(QFileInfo(path).fileName())
                .arg(dims.size()));
        });

        resize(800, 600);
    }

private:
    SvgPreviewWidget* m_preview;
    QLabel* m_statusLabel;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    IntegrationDemo demo;
    demo.show();
    return app.exec();
}

#include "main.moc"
