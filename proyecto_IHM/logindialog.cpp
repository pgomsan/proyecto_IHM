#include "logindialog.h"
#include "ui_logindialog.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QToolButton>

#include "uiiconutils.h"

namespace {
void repolish(QWidget *widget)
{
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
}

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(tr("Iniciar sesión"));

    ui->titleLabel->setProperty("role", "title");
    ui->cardFrame->setProperty("cssClass", "card");
    ui->togglePasswordButton->setProperty("role", "subtle");
    ui->confirmButton->setProperty("role", "primary");
    ui->registerHintLabel->setProperty("role", "hint");
    ui->registerButton->setProperty("role", "secondary");

    repolish(ui->titleLabel);
    repolish(ui->cardFrame);
    repolish(ui->togglePasswordButton);
    repolish(ui->confirmButton);
    repolish(ui->registerHintLabel);
    repolish(ui->registerButton);

    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->togglePasswordButton->setCheckable(true);
    ui->togglePasswordButton->setChecked(false);
    ui->togglePasswordButton->setText(QString());
    ui->togglePasswordButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui->togglePasswordButton->setIconSize(QSize(18, 18));
    ui->togglePasswordButton->setFixedSize(QSize(40, 32));
    ui->togglePasswordButton->setIcon(makeFixedColorSvgIcon(":/icons/eye.svg", QSize(18, 18)));
    ui->togglePasswordButton->setToolTip(tr("Mostrar contraseña"));

    connect(ui->togglePasswordButton, &QToolButton::toggled,
            this, &LoginDialog::togglePasswordVisibility);
    connect(ui->confirmButton, &QPushButton::clicked,
            this, &LoginDialog::handleConfirm);
    connect(ui->registerButton, &QPushButton::clicked, this, [this]() {
        emit registerRequested();
    });
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::handleConfirm()
{
    const QString username = ui->usernameLineEdit->text().trimmed();
    const QString password = ui->passwordLineEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Campos incompletos"),
                             tr("Introduce usuario y contraseña."));
        return;
    }

    emit loginRequested(username, password);
}

void LoginDialog::togglePasswordVisibility(bool checked)
{
    ui->passwordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->togglePasswordButton->setIcon(makeFixedColorSvgIcon(
        checked ? ":/icons/eye-off.svg" : ":/icons/eye.svg", QSize(18, 18)));
    ui->togglePasswordButton->setToolTip(
        checked ? tr("Ocultar contraseña") : tr("Mostrar contraseña"));
}
