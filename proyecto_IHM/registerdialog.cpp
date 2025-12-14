#include "registerdialog.h"
#include "ui_registerdialog.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QImageReader>
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

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(tr("Crear cuenta"));

    ui->titleLabel->setProperty("role", "title");
    ui->cardFrame->setProperty("cssClass", "card");
    ui->avatarPlaceholder->setProperty("role", "avatarPlaceholder");
    ui->usernameHintLabel->setProperty("role", "hint");
    ui->togglePasswordButton->setProperty("role", "subtle");
    ui->passwordHintLabel->setProperty("role", "hint");
    ui->confirmButton->setProperty("role", "primary");
    ui->chooseAvatarButton->setProperty("role", "secondary");
    ui->cancelButton->setProperty("role", "secondary");

    repolish(ui->titleLabel);
    repolish(ui->cardFrame);
    repolish(ui->avatarPlaceholder);
    repolish(ui->usernameHintLabel);
    repolish(ui->togglePasswordButton);
    repolish(ui->passwordHintLabel);
    repolish(ui->confirmButton);
    repolish(ui->chooseAvatarButton);
    repolish(ui->cancelButton);

    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->togglePasswordButton->setCheckable(true);
    ui->togglePasswordButton->setChecked(false);
    ui->togglePasswordButton->setText(tr("Ver"));

    ui->birthdateEdit->setDisplayFormat("yyyy-MM-dd");
    ui->birthdateEdit->setCalendarPopup(true);

    connect(ui->togglePasswordButton, &QToolButton::toggled,
            this, &RegisterDialog::togglePasswordVisibility);
    connect(ui->chooseAvatarButton, &QPushButton::clicked,
            this, &RegisterDialog::chooseAvatar);
    connect(ui->cancelButton, &QPushButton::clicked,
            this, &RegisterDialog::reject);
    connect(ui->confirmButton, &QPushButton::clicked,
            this, &RegisterDialog::onConfirm);

    updateAvatarPreview();
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

void RegisterDialog::chooseAvatar()
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

void RegisterDialog::updateAvatarPreview()
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

void RegisterDialog::onConfirm()
{
    // Solo interfaz: emitimos señal y cerramos; validaciones vendrán después.
    emit registerRequested(ui->usernameLineEdit->text().trimmed(),
                           ui->passwordLineEdit->text(),
                           ui->emailLineEdit->text().trimmed(),
                           ui->birthdateEdit->date(),
                           m_avatar);
    accept();
}
