#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
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
                        const QDate &birthdate);
    void logoutRequested();

private slots:
    void togglePasswordVisibility(bool checked);
    void onConfirm();
    void onCancel();
    void onLogout();

private:
    Ui::ProfileDialog *ui;
};

#endif // PROFILEDIALOG_H
