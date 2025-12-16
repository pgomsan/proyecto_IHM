#include "profiledialog.h"
#include "ui_profiledialog.h"

#include <QFileDialog>
#include <QImageReader>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
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

ProfileDialog::ProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(tr("Perfil"));

    ui->titleLabel->setProperty("role", "title");
    ui->cardFrame->setProperty("cssClass", "card");
    ui->avatarPlaceholder->setProperty("role", "avatarPlaceholder");
    ui->usernameHintLabel->setProperty("role", "hint");
    ui->togglePasswordButton->setProperty("role", "subtle");
    ui->logoutButton->setProperty("role", "danger");
    ui->cancelButton->setProperty("role", "secondary");
    ui->confirmButton->setProperty("role", "primary");
    ui->chooseAvatarButton->setProperty("role", "secondary");

    repolish(ui->titleLabel);
    repolish(ui->cardFrame);
    repolish(ui->avatarPlaceholder);
    repolish(ui->usernameHintLabel);
    repolish(ui->togglePasswordButton);
    repolish(ui->logoutButton);
    repolish(ui->cancelButton);
    repolish(ui->confirmButton);
    repolish(ui->chooseAvatarButton);

    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->togglePasswordButton->setCheckable(true);
    ui->togglePasswordButton->setChecked(false);
    ui->togglePasswordButton->setText(QString());
    ui->togglePasswordButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui->togglePasswordButton->setIconSize(QSize(18, 18));
    ui->togglePasswordButton->setFixedSize(QSize(40, 32));
    ui->togglePasswordButton->setIcon(makeFixedColorSvgIcon(":/icons/eye.svg", QSize(18, 18)));
    ui->togglePasswordButton->setToolTip(tr("Mostrar contraseña"));
    ui->birthdateEdit->setCalendarPopup(true);
    ui->birthdateEdit->setDisplayFormat("yyyy-MM-dd");

    connect(ui->togglePasswordButton, &QToolButton::toggled,
            this, &ProfileDialog::togglePasswordVisibility);
    connect(ui->chooseAvatarButton, &QPushButton::clicked,
            this, &ProfileDialog::chooseAvatar);
    connect(ui->confirmButton, &QPushButton::clicked,
            this, &ProfileDialog::onConfirm);
    connect(ui->cancelButton, &QPushButton::clicked,
            this, &ProfileDialog::onCancel);
    connect(ui->logoutButton, &QPushButton::clicked,
            this, &ProfileDialog::onLogout);

    updateAvatarPreview();
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
    m_avatar = user->avatar();
    updateAvatarPreview();
}

void ProfileDialog::togglePasswordVisibility(bool checked)
{
    ui->passwordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->togglePasswordButton->setIcon(makeFixedColorSvgIcon(
        checked ? ":/icons/eye-off.svg" : ":/icons/eye.svg", QSize(18, 18)));
    ui->togglePasswordButton->setToolTip(
        checked ? tr("Ocultar contraseña") : tr("Mostrar contraseña"));
}

void ProfileDialog::chooseAvatar()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Selecciona una foto de perfil"),
        QString(),
        tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.webp);;Todos los archivos (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Foto de perfil"),
                             tr("No se pudo cargar la imagen seleccionada."));
        return;
    }

    m_avatar = img;
    updateAvatarPreview();
}

void ProfileDialog::updateAvatarPreview()
{
    const QSize targetSize = ui->avatarPlaceholder->size().isValid()
            ? ui->avatarPlaceholder->size()
            : QSize(140, 140);

    QPixmap pix;
    if (!m_avatar.isNull()) {
        pix = QPixmap::fromImage(m_avatar);
    } else {
        pix.load(":/icons/sinfotodeperfil.png");
    }

    if (!pix.isNull()) {
        ui->avatarPlaceholder->setPixmap(
            pix.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        ui->avatarPlaceholder->setText(tr("[ Sin foto ]"));
    }
}

void ProfileDialog::onConfirm()
{
    emit profileUpdated(ui->passwordLineEdit->text(),
                        ui->emailLineEdit->text().trimmed(),
                        ui->birthdateEdit->date(),
                        m_avatar);
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
