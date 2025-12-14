#include "historydialog.h"
#include "ui_historydialog.h"

#include <QDateTime>
#include <QStyle>

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

HistoryDialog::HistoryDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::HistoryDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(tr("Historial"));
    setMinimumSize(620, 520);

    ui->titleLabel->setProperty("role", "title");
    ui->filtersCard->setProperty("cssClass", "card");
    ui->resultsCard->setProperty("cssClass", "card");
    ui->totalsCard->setProperty("cssClass", "card");
    ui->sessionsStatFrame->setProperty("cssClass", "card");
    ui->hitsStatFrame->setProperty("cssClass", "card");
    ui->faultsStatFrame->setProperty("cssClass", "card");
    ui->hintLabel->setProperty("role", "hint");
    ui->applyButton->setProperty("role", "primary");
    ui->closeButton->setProperty("role", "secondary");

    repolish(ui->titleLabel);
    repolish(ui->filtersCard);
    repolish(ui->resultsCard);
    repolish(ui->totalsCard);
    repolish(ui->sessionsStatFrame);
    repolish(ui->hitsStatFrame);
    repolish(ui->faultsStatFrame);
    repolish(ui->hintLabel);
    repolish(ui->applyButton);
    repolish(ui->closeButton);

    ui->summaryTitleLabel->setProperty("role", "hint");
    ui->sessionsValueLabel->setProperty("role", "statValue");
    ui->hitsValueLabel->setProperty("role", "statValue");
    ui->faultsValueLabel->setProperty("role", "statValue");
    ui->sessionsLabel->setProperty("role", "statLabel");
    ui->hitsLabel->setProperty("role", "statLabel");
    ui->faultsLabel->setProperty("role", "statLabel");

    repolish(ui->summaryTitleLabel);
    repolish(ui->sessionsValueLabel);
    repolish(ui->hitsValueLabel);
    repolish(ui->faultsValueLabel);
    repolish(ui->sessionsLabel);
    repolish(ui->hitsLabel);
    repolish(ui->faultsLabel);

    ui->fromDateEdit->setCalendarPopup(true);
    ui->toDateEdit->setCalendarPopup(true);
    ui->fromDateEdit->setDisplayFormat("yyyy-MM-dd");
    ui->toDateEdit->setDisplayFormat("yyyy-MM-dd");

    ui->sessionsTable->setColumnCount(3);
    ui->sessionsTable->setHorizontalHeaderLabels(
        {tr("Fecha"), tr("Aciertos"), tr("Fallos")});
    ui->sessionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->sessionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->sessionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->sessionsTable->setAlternatingRowColors(true);
    ui->sessionsTable->horizontalHeader()->setStretchLastSection(true);
    ui->sessionsTable->verticalHeader()->setVisible(false);

    connect(ui->applyButton, &QPushButton::clicked, this, &HistoryDialog::applyFilter);
    connect(ui->closeButton, &QPushButton::clicked, this, &HistoryDialog::reject);
}

HistoryDialog::~HistoryDialog()
{
    delete ui;
}

void HistoryDialog::setSessions(const QVector<Session> &sessions)
{
    m_sessions.clear();
    m_sessions.reserve(sessions.size());
    for (const auto &s : sessions) {
        if (s.timeStamp().isValid()) {
            m_sessions.push_back(s);
        }
    }

    if (m_sessions.isEmpty()) {
        const auto today = QDate::currentDate();
        ui->fromDateEdit->setDate(today);
        ui->toDateEdit->setDate(today);
        ui->hintLabel->setText(tr("Todavía no hay sesiones registradas."));
        updateTable({});
        updateTotals({});
        return;
    }

    const QDate minDate = m_sessions.first().timeStamp().date();
    const QDate maxDate = m_sessions.last().timeStamp().date();

    ui->fromDateEdit->setDate(minDate);
    ui->toDateEdit->setDate(maxDate);
    ui->hintLabel->setText(tr("Filtra por fechas para consultar tus sesiones."));
    applyFilter();
}

void HistoryDialog::applyFilter()
{
    const QVector<Session> sessions = filteredSessions();
    updateTable(sessions);
    updateTotals(sessions);

    if (sessions.isEmpty() && !m_sessions.isEmpty()) {
        ui->hintLabel->setText(tr("No hay sesiones en el rango seleccionado."));
    } else if (!m_sessions.isEmpty()) {
        ui->hintLabel->setText(tr("Filtra por fechas para consultar tus sesiones."));
    }
}

QVector<Session> HistoryDialog::filteredSessions() const
{
    if (m_sessions.isEmpty()) {
        return {};
    }

    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    if (!from.isValid() || !to.isValid()) {
        return m_sessions;
    }
    if (from > to) {
        std::swap(from, to);
    }

    const QDateTime fromDt(from, QTime(0, 0, 0));
    const QDateTime toDt(to, QTime(23, 59, 59));

    QVector<Session> out;
    out.reserve(m_sessions.size());
    for (const auto &s : m_sessions) {
        const QDateTime ts = s.timeStamp();
        if (!ts.isValid()) {
            continue;
        }
        if (ts >= fromDt && ts <= toDt) {
            out.push_back(s);
        }
    }
    return out;
}

void HistoryDialog::updateTable(const QVector<Session> &sessions)
{
    ui->sessionsTable->setRowCount(sessions.size());

    for (int row = 0; row < sessions.size(); ++row) {
        const Session &s = sessions.at(row);
        const QString ts = s.timeStamp().toString("yyyy-MM-dd HH:mm");

        auto *dateItem = new QTableWidgetItem(ts);
        dateItem->setData(Qt::UserRole, s.timeStamp());

        auto *hitsItem = new QTableWidgetItem(QString::number(s.hits()));
        hitsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto *faultsItem = new QTableWidgetItem(QString::number(s.faults()));
        faultsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        ui->sessionsTable->setItem(row, 0, dateItem);
        ui->sessionsTable->setItem(row, 1, hitsItem);
        ui->sessionsTable->setItem(row, 2, faultsItem);
    }

    ui->sessionsTable->resizeColumnsToContents();
}

void HistoryDialog::updateTotals(const QVector<Session> &sessions)
{
    int totalHits = 0;
    int totalFaults = 0;
    for (const auto &s : sessions) {
        totalHits += s.hits();
        totalFaults += s.faults();
    }

    ui->sessionsValueLabel->setText(QString::number(sessions.size()));
    ui->hitsValueLabel->setText(QString::number(totalHits));
    ui->faultsValueLabel->setText(QString::number(totalFaults));
}
