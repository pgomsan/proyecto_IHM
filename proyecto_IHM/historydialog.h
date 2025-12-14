#pragma once

#include <QDialog>
#include <QVector>

#include "navdb/lib/include/navtypes.h"

namespace Ui {
class HistoryDialog;
}

class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(QWidget *parent = nullptr);
    ~HistoryDialog() override;

    void setSessions(const QVector<Session> &sessions);

private slots:
    void applyFilter();

private:
    void updateTable(const QVector<Session> &sessions);
    void updateTotals(const QVector<Session> &sessions);
    QVector<Session> filteredSessions() const;

    Ui::HistoryDialog *ui;
    QVector<Session> m_sessions;
};

