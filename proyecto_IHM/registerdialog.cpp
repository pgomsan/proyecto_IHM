#include "registerdialog.h"
#include "ui_registerdialog.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QImageReader>
#include <QStyle>
#include <QToolButton>
#include <QRegularExpression>
#include <QStringList>

#include "navdb/lib/include/navigation.h"

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

int calculateAge(const QDate &birthdate, const QDate &today)
{
    if (!birthdate.isValid() || !today.isValid() || birthdate > today) {
        return 0;
    }
    int age = today.year() - birthdate.year();
    QDate birthdayThisYear(today.year(), birthdate.month(), birthdate.day());
    if (!birthdayThisYear.isValid()) {
        // e.g. 29/02 on non-leap years -> clamp to last day of month
        birthdayThisYear = QDate(today.year(), birthdate.month(), 1).addMonths(1).addDays(-1);
    }
    if (today < birthdayThisYear) {
        age -= 1;
    }
    return age;
}

QString allowedSymbols()
{
    return QStringLiteral("!@#$%&*()-+=");
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
    ui->emailHintLabel->setProperty("role", "hint");
    ui->birthdateHintLabel->setProperty("role", "hint");
    ui->confirmButton->setProperty("role", "primary");
    ui->chooseAvatarButton->setProperty("role", "secondary");
    ui->cancelButton->setProperty("role", "secondary");

    repolish(ui->titleLabel);
    repolish(ui->cardFrame);
    repolish(ui->avatarPlaceholder);
    repolish(ui->usernameHintLabel);
    repolish(ui->togglePasswordButton);
    repolish(ui->passwordHintLabel);
    repolish(ui->emailHintLabel);
    repolish(ui->birthdateHintLabel);
    repolish(ui->confirmButton);
    repolish(ui->chooseAvatarButton);
    repolish(ui->cancelButton);

    m_defaultUsernameHint = ui->usernameHintLabel->text();
    m_defaultPasswordHint = ui->passwordHintLabel->text();
    m_defaultEmailHint = ui->emailHintLabel->text();
    m_defaultBirthdateHint = ui->birthdateHintLabel->text();

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

    connect(ui->usernameLineEdit, &QLineEdit::textEdited, this, [this]() {
        m_usernameTouched = true;
        updateValidation();
    });
    connect(ui->passwordLineEdit, &QLineEdit::textEdited, this, [this]() {
        m_passwordTouched = true;
        updateValidation();
    });
    connect(ui->emailLineEdit, &QLineEdit::textEdited, this, [this]() {
        m_emailTouched = true;
        updateValidation();
    });
    connect(ui->birthdateEdit, &QDateEdit::dateChanged, this, [this]() {
        m_birthdateTouched = true;
        updateValidation();
    });

    updateAvatarPreview();
    updateValidation();
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
    if (!validateAll(true)) {
        return;
    }

    emit registerRequested(ui->usernameLineEdit->text().trimmed(),
                           ui->passwordLineEdit->text(),
                           ui->emailLineEdit->text().trimmed(),
                           ui->birthdateEdit->date(),
                           m_avatar);
    accept();
}

void RegisterDialog::updateValidation()
{
    const QString username = ui->usernameLineEdit->text();
    const QString password = ui->passwordLineEdit->text();
    const QString email = ui->emailLineEdit->text();
    const QDate birthdate = ui->birthdateEdit->date();

    bool usernameOk = false;
    bool passwordOk = false;
    bool emailOk = false;
    bool birthdateOk = false;

    // Usuario
    if (!m_usernameTouched && username.trimmed().isEmpty()) {
        setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, QString(), m_defaultUsernameHint);
    } else if (username.trimmed().isEmpty()) {
        setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "error", tr("El usuario es obligatorio."));
    } else {
        const QString trimmed = username.trimmed();
        if (trimmed.size() < 6) {
            setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "error",
                          tr("Debe tener al menos 6 caracteres."));
        } else if (trimmed.size() > 15) {
            setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "error",
                          tr("Debe tener como máximo 15 caracteres."));
        } else if (trimmed.contains(' ') || trimmed.contains('-') || trimmed.contains('_')) {
            setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "error",
                          tr("No se permiten espacios, '-' ni '_' ."));
        } else {
            static const QRegularExpression kUserRx(QStringLiteral("^[A-Za-z0-9]{6,15}$"));
            if (!kUserRx.match(trimmed).hasMatch()) {
                setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "error",
                              tr("Solo letras y números (sin acentos)."));
            } else if (Navigation::instance().findUser(trimmed)) {
                setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "error",
                              tr("Ese usuario ya existe."));
            } else {
                usernameOk = true;
                setFieldState(ui->usernameLineEdit, ui->usernameHintLabel, "ok",
                              tr("Usuario válido."));
            }
        }
    }

    // Contraseña
    if (!m_passwordTouched && password.isEmpty()) {
        setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, QString(), m_defaultPasswordHint);
    } else if (password.isEmpty()) {
        setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, "error", tr("La contraseña es obligatoria."));
    } else {
        const int length = password.size();
        if (length < 8) {
            setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, "error",
                          tr("Debe tener al menos 8 caracteres."));
        } else if (length > 20) {
            setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, "error",
                          tr("Debe tener como máximo 20 caracteres."));
        } else if (password.contains(QRegularExpression(QStringLiteral("\\s")))) {
            setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, "error",
                          tr("No se permiten espacios."));
        } else {
            bool hasUpper = false;
            bool hasLower = false;
            bool hasDigit = false;
            bool hasSymbol = false;
            const QString symbols = allowedSymbols();
            for (const QChar ch : password) {
                if (ch.isUpper()) {
                    hasUpper = true;
                } else if (ch.isLower()) {
                    hasLower = true;
                } else if (ch.isDigit()) {
                    hasDigit = true;
                }
                if (symbols.contains(ch)) {
                    hasSymbol = true;
                }
            }

            QStringList missing;
            if (!hasUpper) {
                missing << tr("una mayúscula");
            }
            if (!hasLower) {
                missing << tr("una minúscula");
            }
            if (!hasDigit) {
                missing << tr("un dígito");
            }
            if (!hasSymbol) {
                missing << tr("un símbolo (%1)").arg(symbols);
            }

            if (!missing.isEmpty()) {
                setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, "error",
                              tr("Falta %1.").arg(missing.join(", ")));
            } else {
                passwordOk = true;
                setFieldState(ui->passwordLineEdit, ui->passwordHintLabel, "ok",
                              tr("Contraseña segura."));
            }
        }
    }

    // Email
    if (!m_emailTouched && email.trimmed().isEmpty()) {
        setFieldState(ui->emailLineEdit, ui->emailHintLabel, QString(), m_defaultEmailHint);
    } else if (email.trimmed().isEmpty()) {
        setFieldState(ui->emailLineEdit, ui->emailHintLabel, "error", tr("El email es obligatorio."));
    } else if (email.contains(QRegularExpression(QStringLiteral("\\s")))) {
        setFieldState(ui->emailLineEdit, ui->emailHintLabel, "error", tr("No se permiten espacios."));
    } else {
        const QString trimmed = email.trimmed();
        static const QRegularExpression kEmailRx(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        if (!trimmed.contains('@')) {
            setFieldState(ui->emailLineEdit, ui->emailHintLabel, "error", tr("Debe contener '@'."));
        } else if (!kEmailRx.match(trimmed).hasMatch()) {
            setFieldState(ui->emailLineEdit, ui->emailHintLabel, "error", tr("Formato de email no válido."));
        } else {
            emailOk = true;
            setFieldState(ui->emailLineEdit, ui->emailHintLabel, "ok", tr("Email válido."));
        }
    }

    // Fecha de nacimiento
    if (!m_birthdateTouched) {
        setFieldState(ui->birthdateEdit, ui->birthdateHintLabel, QString(), m_defaultBirthdateHint);
    } else {
        const QDate today = QDate::currentDate();
        const int age = calculateAge(birthdate, today);
        if (age <= 16) {
            setFieldState(ui->birthdateEdit, ui->birthdateHintLabel, "error",
                          tr("Debes ser mayor de 16 años (edad actual: %1).").arg(age));
        } else {
            birthdateOk = true;
            setFieldState(ui->birthdateEdit, ui->birthdateHintLabel, "ok",
                          tr("Edad válida (%1 años).").arg(age));
        }
    }

    m_formValid = usernameOk && passwordOk && emailOk && birthdateOk;
}

bool RegisterDialog::validateAll(bool showMessageBox)
{
    if (showMessageBox) {
        m_usernameTouched = true;
        m_passwordTouched = true;
        m_emailTouched = true;
        m_birthdateTouched = true;
    }

    updateValidation();

    if (m_formValid) {
        return true;
    }

    if (showMessageBox) {
        QMessageBox::warning(this, tr("Revisa los campos"),
                             tr("Hay campos incorrectos. Corrígelos para crear la cuenta."));
    }
    return false;
}

void RegisterDialog::setFieldState(QWidget *field, QLabel *hintLabel, const QString &state, const QString &message)
{
    if (field) {
        if (state.isEmpty()) {
            field->setProperty("validationState", QVariant());
        } else {
            field->setProperty("validationState", state);
        }
        repolish(field);
    }
    if (hintLabel) {
        if (state.isEmpty()) {
            hintLabel->setProperty("validationState", QVariant());
        } else {
            hintLabel->setProperty("validationState", state);
        }
        hintLabel->setText(message);
        repolish(hintLabel);
    }
}
