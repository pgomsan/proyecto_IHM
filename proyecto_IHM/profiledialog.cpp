#include "profiledialog.h"
#include "ui_profiledialog.h"

#include <QLineEdit>
#include <QToolButton>

ProfileDialog::ProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(tr("Perfil"));

    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->togglePasswordButton->setCheckable(true);
    ui->togglePasswordButton->setChecked(false);
    ui->togglePasswordButton->setText(tr("Ver"));
    ui->birthdateEdit->setCalendarPopup(true);
    ui->birthdateEdit->setDisplayFormat("yyyy-MM-dd");

    connect(ui->togglePasswordButton, &QToolButton::toggled,
            this, &ProfileDialog::togglePasswordVisibility);
    connect(ui->confirmButton, &QPushButton::clicked,
            this, &ProfileDialog::onConfirm);
    connect(ui->cancelButton, &QPushButton::clicked,
            this, &ProfileDialog::onCancel);
    connect(ui->logoutButton, &QPushButton::clicked,
            this, &ProfileDialog::onLogout);
}

ProfileDialog::~ProfileDialog()
{
    delete ui;
}

void ProfileDialog::setUser(const User *user)
{
    if (!user) {
        return;
    }
    ui->usernameValue->setText(user->nickName());
    ui->passwordLineEdit->setText(user->password());
    ui->emailLineEdit->setText(user->email());
    ui->birthdateEdit->setDate(user->birthdate());
    // Avatar opcional: por ahora placeholder textual.
}

void ProfileDialog::togglePasswordVisibility(bool checked)
{
    ui->passwordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->togglePasswordButton->setText(checked ? tr("Ocultar") : tr("Ver"));
}

void ProfileDialog::onConfirm()
{
    emit profileUpdated(ui->passwordLineEdit->text(),
                        ui->emailLineEdit->text().trimmed(),
                        ui->birthdateEdit->date());
    accept();
}

void ProfileDialog::onCancel()
{
    reject();
}

void ProfileDialog::onLogout()
{
    emit logoutRequested();
    accept();
}
