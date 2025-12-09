#include "registerdialog.h"
#include "ui_registerdialog.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QToolButton>

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(tr("Registrarse"));

    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->togglePasswordButton->setCheckable(true);
    ui->togglePasswordButton->setChecked(false);
    ui->togglePasswordButton->setText(tr("Ver"));

    ui->birthdateEdit->setDisplayFormat("yyyy-MM-dd");
    ui->birthdateEdit->setCalendarPopup(true);

    connect(ui->togglePasswordButton, &QToolButton::toggled,
            this, &RegisterDialog::togglePasswordVisibility);
    connect(ui->confirmButton, &QPushButton::clicked,
            this, &RegisterDialog::onConfirm);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::togglePasswordVisibility(bool checked)
{
    ui->passwordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->togglePasswordButton->setText(checked ? tr("Ocultar") : tr("Ver"));
}

void RegisterDialog::onConfirm()
{
    // Solo interfaz: emitimos señal y cerramos; validaciones vendrán después.
    emit registerRequested(ui->usernameLineEdit->text().trimmed(),
                           ui->passwordLineEdit->text(),
                           ui->emailLineEdit->text().trimmed(),
                           ui->birthdateEdit->date());
    accept();
}
