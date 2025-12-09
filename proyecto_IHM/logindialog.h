#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void loginRequested(const QString &username, const QString &password);
    void registerRequested();

private slots:
    void handleConfirm();
    void togglePasswordVisibility(bool checked);
private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
