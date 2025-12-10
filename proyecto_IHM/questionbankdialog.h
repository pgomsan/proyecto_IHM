#ifndef QUESTIONBANKDIALOG_H
#define QUESTIONBANKDIALOG_H

#include <QDialog>
#include <QVector>
#include "navdb/lib/include/navtypes.h"

class QListWidget;
class QListWidgetItem;

class QuestionBankDialog : public QDialog
{
    Q_OBJECT
public:
    QuestionBankDialog(const QVector<Problem> &problems, QWidget *parent = nullptr);

signals:
    void problemSelected(const Problem &problem);

private slots:
    void handleOpenSelected();
    void handleItemActivated(QListWidgetItem *item);

private:
    void populateList();
    const Problem *problemForItem(QListWidgetItem *item) const;

    QVector<Problem> m_problems;
    QListWidget *listWidget = nullptr;
};

#endif // QUESTIONBANKDIALOG_H
