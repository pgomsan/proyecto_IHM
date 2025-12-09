#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QString>

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
                           const QDate &birthdate);

private slots:
    void togglePasswordVisibility(bool checked);
    void onConfirm();

private:
    Ui::RegisterDialog *ui;
};

#endif // REGISTERDIALOG_H
