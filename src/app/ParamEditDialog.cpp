// src/app/ParamEditDialog.cpp
#include "app/ParamEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

ParamEditDialog::ParamEditDialog(const Dimension* dim, QWidget* parent)
    : QDialog(parent), m_dim(dim) {
    setWindowTitle(QString("参数绑定 — %1").arg(QString::fromStdString(dim->id)));
    setFixedSize(320, 280);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // Dimension info
    auto* infoBox = new QGroupBox("尺寸信息", this);
    auto* infoForm = new QFormLayout(infoBox);

    auto dimTypeStr = [](const std::string& t) -> QString {
        if (t == "aligned") return "对齐标注";
        if (t == "linear") return "线性标注";
        if (t == "radial") return "半径标注";
        if (t == "diameter") return "直径标注";
        if (t == "angular") return "角度标注";
        return QString::fromStdString(t);
    };

    infoForm->addRow("ID:", new QLabel(QString::fromStdString(dim->id), this));
    infoForm->addRow("类型:", new QLabel(dimTypeStr(dim->type), this));
    infoForm->addRow("值:", new QLabel(QString::number(dim->value) + " " +
                                        QString::fromStdString(dim->unit), this));
    infoForm->addRow("中心:", new QLabel(
        QString("(%1, %2)").arg(dim->centerX).arg(dim->centerY), this));
    layout->addWidget(infoBox);

    // Parameter binding
    auto* paramBox = new QGroupBox("参数绑定", this);
    auto* paramLayout = new QVBoxLayout(paramBox);

    // Type toggle
    auto* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("参数类型:", this));

    m_floatBtn = new QPushButton("float", this);
    m_floatBtn->setCheckable(true);
    m_floatBtn->setChecked(true);
    m_floatBtn->setStyleSheet(
        "QPushButton{background:rgba(79,195,247,0.1);border:1px solid rgba(79,195,247,0.3);"
        "border-radius:4px;padding:6px 16px;color:#4fc3f7;font-weight:600;}"
        "QPushButton:checked{background:rgba(79,195,247,0.2);border-color:#4fc3f7;}");

    m_intBtn = new QPushButton("int", this);
    m_intBtn->setCheckable(true);
    m_intBtn->setStyleSheet(
        "QPushButton{background:rgba(76,175,80,0.1);border:1px solid rgba(76,175,80,0.3);"
        "border-radius:4px;padding:6px 16px;color:#4caf50;font-weight:600;}"
        "QPushButton:checked{background:rgba(76,175,80,0.2);border-color:#4caf50;}");

    connect(m_floatBtn, &QPushButton::clicked, this, [this]() {
        m_floatBtn->setChecked(true); m_intBtn->setChecked(false);
        m_selectedType = "float";
    });
    connect(m_intBtn, &QPushButton::clicked, this, [this]() {
        m_floatBtn->setChecked(false); m_intBtn->setChecked(true);
        m_selectedType = "int";
    });

    typeLayout->addWidget(m_floatBtn);
    typeLayout->addWidget(m_intBtn);
    typeLayout->addStretch();
    paramLayout->addLayout(typeLayout);

    // Key input
    auto* keyLayout = new QHBoxLayout();
    keyLayout->addWidget(new QLabel("参数 Key:", this));
    m_keyEdit = new QLineEdit(this);
    m_keyEdit->setPlaceholderText("key_name");
    m_keyEdit->setStyleSheet(
        "QLineEdit{background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.1);"
        "border-radius:4px;padding:6px 8px;color:#fff;font-family:monospace;}");
    keyLayout->addWidget(m_keyEdit, 1);
    paramLayout->addLayout(keyLayout);

    // Default value
    auto* defaultLayout = new QHBoxLayout();
    defaultLayout->addWidget(new QLabel("默认值:", this));
    m_defaultEdit = new QLineEdit(this);
    m_defaultEdit->setText(QString::number(dim->value));
    m_defaultEdit->setStyleSheet(m_keyEdit->styleSheet());
    defaultLayout->addWidget(m_defaultEdit, 1);
    paramLayout->addLayout(defaultLayout);

    // Pre-fill if already bound
    if (dim->isBound()) {
        m_keyEdit->setText(QString::fromStdString(dim->param->key));
        m_defaultEdit->setText(QString::number(dim->param->defaultValue));
        if (dim->param->type == "int") {
            m_floatBtn->setChecked(false);
            m_intBtn->setChecked(true);
            m_selectedType = "int";
        }
    }

    layout->addWidget(paramBox);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* bindBtn = new QPushButton(dim->isBound() ? "✓ 更新" : "✓ 绑定", this);
    bindBtn->setStyleSheet(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #4fc3f7,stop:1 #7c4dff);border:none;border-radius:4px;"
        "padding:8px 24px;color:#fff;font-weight:600;}");
    connect(bindBtn, &QPushButton::clicked, this, &ParamEditDialog::onBind);

    auto* unbindBtn = new QPushButton("解绑", this);
    unbindBtn->setStyleSheet(
        "QPushButton{background:transparent;border:1px solid rgba(255,255,255,0.1);"
        "border-radius:4px;padding:8px 24px;color:rgba(255,255,255,0.4);}");
    connect(unbindBtn, &QPushButton::clicked, this, &ParamEditDialog::onUnbind);

    auto* cancelBtn = new QPushButton("取消", this);
    cancelBtn->setStyleSheet(unbindBtn->styleSheet());
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(bindBtn);
    btnLayout->addWidget(unbindBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    if (!dim->isBound()) unbindBtn->setEnabled(false);
}

void ParamEditDialog::onBind() {
    QString key = m_keyEdit->text().trimmed();
    if (key.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入参数 Key");
        m_keyEdit->setFocus();
        return;
    }

    m_result.bind = true;
    m_result.type = m_selectedType.toStdString();
    m_result.key = key.toStdString();
    m_result.defaultValue = m_defaultEdit->text().toDouble();
    accept();
}

void ParamEditDialog::onUnbind() {
    m_result.bind = false;
    accept();
}
