#include "questionbankdialog.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

QuestionBankDialog::QuestionBankDialog(const QVector<Problem> &problems, QWidget *parent)
    : QDialog(parent),
      m_problems(problems)
{
    setWindowTitle(tr("Banco de preguntas"));
    setModal(true);
    setMinimumSize(420, 480);

    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(tr("Selecciona el enunciado que quieras practicar"), this);
    title->setWordWrap(true);
    layout->addWidget(title);

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget->setUniformItemSizes(true);
    listWidget->setAlternatingRowColors(true);
    listWidget->setWordWrap(true);
    populateList();
    connect(listWidget, &QListWidget::itemDoubleClicked,
            this, &QuestionBankDialog::handleItemActivated);
    layout->addWidget(listWidget, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *openButton = buttons->addButton(tr("Abrir pregunta"), QDialogButtonBox::ActionRole);
    connect(openButton, &QPushButton::clicked, this, &QuestionBankDialog::handleOpenSelected);
    connect(buttons, &QDialogButtonBox::rejected, this, &QuestionBankDialog::reject);
    layout->addWidget(buttons);
}

void QuestionBankDialog::populateList()
{
    listWidget->clear();
    for (int i = 0; i < m_problems.size(); ++i) {
        const auto &problem = m_problems.at(i);
        auto *item = new QListWidgetItem(problem.text(), listWidget);
        item->setData(Qt::UserRole, i);
        item->setToolTip(problem.text());
        listWidget->addItem(item);
    }
}

void QuestionBankDialog::handleOpenSelected()
{
    auto *item = listWidget->currentItem();
    handleItemActivated(item);
}

void QuestionBankDialog::handleItemActivated(QListWidgetItem *item)
{
    const Problem *problem = problemForItem(item);
    if (!problem) {
        return;
    }
    emit problemSelected(*problem);
    accept();
}

const Problem *QuestionBankDialog::problemForItem(QListWidgetItem *item) const
{
    if (!item) {
        return nullptr;
    }
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_problems.size()) {
        return nullptr;
    }
    return &m_problems.at(idx);
}
