#include "logindialog.h"
#include "ui_logindialog.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QStyle>
#include <QToolButton>

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
    ui->togglePasswordButton->setText(tr("Ver"));

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
    accept();
}

void LoginDialog::togglePasswordVisibility(bool checked)
{
    ui->passwordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->togglePasswordButton->setText(checked ? tr("Ocultar") : tr("Ver"));
}
