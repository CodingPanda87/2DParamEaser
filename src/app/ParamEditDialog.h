// src/app/ParamEditDialog.h
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "core/Dimension.h"

struct ParamEditResult {
    bool bind = false;
    std::string type = "float";
    std::string key;
    double defaultValue = 0.0;
};

class ParamEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ParamEditDialog(const Dimension* dim, QWidget* parent = nullptr);

    ParamEditResult result() const { return m_result; }

private slots:
    void onBind();
    void onUnbind();

private:
    const Dimension* m_dim;
    ParamEditResult m_result;

    QPushButton* m_floatBtn = nullptr;
    QPushButton* m_intBtn = nullptr;
    QLineEdit* m_keyEdit = nullptr;
    QLineEdit* m_defaultEdit = nullptr;
    QString m_selectedType = "float";
};
