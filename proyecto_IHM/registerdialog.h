#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QString>
#include <QImage>
#include <QDate>
#include <QLabel>

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

signals:
    void registerRequested(const QString &username,
                           const QString &password,
                           const QString &email,
                           const QDate &birthdate,
                           const QImage &avatar);

private slots:
    void togglePasswordVisibility(bool checked);
    void chooseAvatar();
    void onConfirm();
    void updateValidation();

private:
    Ui::RegisterDialog *ui;
    QImage m_avatar;
    bool m_usernameTouched = false;
    bool m_passwordTouched = false;
    bool m_emailTouched = false;
    bool m_birthdateTouched = false;
    bool m_formValid = false;
    QString m_defaultUsernameHint;
    QString m_defaultPasswordHint;
    QString m_defaultEmailHint;
    QString m_defaultBirthdateHint;
    void updateAvatarPreview();
    bool validateAll(bool showMessageBox);
    void setFieldState(QWidget *field, QLabel *hintLabel, const QString &state, const QString &message);
};

#endif // REGISTERDIALOG_H
