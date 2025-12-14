#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include <QImage>
#include "navdb/lib/include/navtypes.h"

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDialog(QWidget *parent = nullptr);
    ~ProfileDialog();

    void setUser(const User *user);

signals:
    void profileUpdated(const QString &password,
                        const QString &email,
                        const QDate &birthdate,
                        const QImage &avatar);
    void logoutRequested();

private slots:
    void togglePasswordVisibility(bool checked);
    void chooseAvatar();
    void onConfirm();
    void onCancel();
    void onLogout();

private:
    Ui::ProfileDialog *ui;
    QImage m_avatar;
    void updateAvatarPreview();
};

#endif // PROFILEDIALOG_H
