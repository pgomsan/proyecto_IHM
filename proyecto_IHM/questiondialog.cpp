#include "questiondialog.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>
#include <random>

ProblemDialog::ProblemDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Pregunta"));
    setModal(false);
    setWindowModality(Qt::NonModal);
    setMinimumSize(260, 160);
    setSizeGripEnabled(true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    setWindowFlag(Qt::Tool, true);

    auto *mainLayout = new QVBoxLayout(this);

    questionLabel = new QLabel(this);
    questionLabel->setWordWrap(true);
    questionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    questionLabel->setMargin(4);
    questionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    mainLayout->addWidget(questionLabel);

    answersContainer = new QWidget(this);
    answersLayout = new QVBoxLayout();
    answersLayout->setSpacing(8);
    answersLayout->setContentsMargins(4, 8, 4, 8);
    answersContainer->setLayout(answersLayout);
    answersScroll = new QScrollArea(this);
    answersScroll->setWidgetResizable(true);
    answersScroll->setWidget(answersContainer);
    answersScroll->setMinimumHeight(80);
    mainLayout->addWidget(answersScroll);

    answerGroup = new QButtonGroup(this);
    answerGroup->setExclusive(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *checkButton = buttonBox->addButton(tr("Comprobar"), QDialogButtonBox::ActionRole);
    QPushButton *toggleAnswers = buttonBox->addButton(tr("Ocultar respuestas"), QDialogButtonBox::ActionRole);
    connect(checkButton, &QPushButton::clicked, this, &ProblemDialog::handleCheckAnswer);
    connect(toggleAnswers, &QPushButton::clicked, this, [this, toggleAnswers]() {
        const bool willHide = answersScroll->isVisible();
        answersScroll->setVisible(!willHide);
        toggleAnswers->setText(willHide ? tr("Mostrar respuestas") : tr("Ocultar respuestas"));
        adjustSize();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProblemDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ProblemDialog::setProblem(const Problem &problem)
{
    questionLabel->setText(problem.text());
    clearAnswers();

    QVector<int> indexes;
    indexes.reserve(problem.answers().size());
    for (int i = 0; i < problem.answers().size(); ++i) {
        indexes.append(i);
    }

    // Mezclamos el orden para cada visualización
    std::shuffle(indexes.begin(), indexes.end(),
                 std::mt19937(QRandomGenerator::global()->generate()));

    for (int idx : indexes) {
        addAnswerOption(problem.answers().at(idx));
    }
}

void ProblemDialog::handleCheckAnswer()
{
    auto *checked = qobject_cast<QRadioButton*>(answerGroup->checkedButton());
    if (!checked) {
        QMessageBox::information(this, tr("Respuesta"),
                                 tr("Selecciona una respuesta para comprobar."));
        return;
    }

    const bool valid = checked->property("valid").toBool();
    const QString msg = valid
            ? tr("Respuesta correcta.")
            : tr("Respuesta incorrecta.");
    QMessageBox::information(this, tr("Resultado"), msg);
}

void ProblemDialog::clearAnswers()
{
    const auto buttons = answerGroup->buttons();
    for (QAbstractButton *btn : buttons) {
        answerGroup->removeButton(btn);
        answersLayout->removeWidget(btn);
        btn->deleteLater();
    }
}

void ProblemDialog::addAnswerOption(const Answer &answer)
{
    auto *radio = new QRadioButton(answer.text(), this);
    radio->setProperty("valid", answer.validity());
    answerGroup->addButton(radio);
    answersLayout->addWidget(radio);
}
