#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QString>
#include <QImage>

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

private:
    Ui::RegisterDialog *ui;
    QImage m_avatar;
    void updateAvatarPreview();
};

#endif // REGISTERDIALOG_H
