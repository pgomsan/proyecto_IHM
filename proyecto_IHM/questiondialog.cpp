#include "questiondialog.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QToolButton>
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
    mainLayout->setContentsMargins(8, 8, 8, 16);
    mainLayout->setSpacing(8);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);
    toggleQuestionButton = new QToolButton(this);
    toggleQuestionButton->setProperty("role", "subtle");
    toggleQuestionButton->setText(tr("Ocultar pregunta"));
    toggleQuestionButton->setAutoRaise(true);
    toggleQuestionButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleQuestionButton->setArrowType(Qt::UpArrow);
    toggleQuestionButton->setCursor(Qt::PointingHandCursor);
    toggleQuestionButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(toggleQuestionButton, &QToolButton::clicked,
            this, &ProblemDialog::toggleQuestionVisibility);
    headerLayout->addWidget(toggleQuestionButton, 1);
    mainLayout->addLayout(headerLayout);

    contentContainer = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(10);

    questionLabel = new QLabel(contentContainer);
    questionLabel->setWordWrap(true);
    questionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    questionLabel->setMargin(4);
    questionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    contentLayout->addWidget(questionLabel);

    answersContainer = new QWidget(contentContainer);
    answersLayout = new QVBoxLayout();
    answersLayout->setSpacing(8);
    answersLayout->setContentsMargins(4, 8, 4, 8);
    answersContainer->setLayout(answersLayout);
    answersScroll = new QScrollArea(contentContainer);
    answersScroll->setWidgetResizable(true);
    answersScroll->setWidget(answersContainer);
    answersScroll->setMinimumHeight(80);
    contentLayout->addWidget(answersScroll);

    mainLayout->addWidget(contentContainer);

    answerGroup = new QButtonGroup(this);
    answerGroup->setExclusive(true);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *checkButton = buttonBox->addButton(tr("Comprobar"), QDialogButtonBox::ActionRole);
    checkButton->setProperty("role", "primary");
    connect(checkButton, &QPushButton::clicked, this, &ProblemDialog::handleCheckAnswer);
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

    // Al cambiar de pregunta, la mostramos (por si estaba colapsada)
    if (questionCollapsed) {
        toggleQuestionVisibility();
    }
}

void ProblemDialog::mousePressEvent(QMouseEvent *event)
{
    if (questionCollapsed && event && event->button() == Qt::LeftButton) {
        toggleQuestionVisibility();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
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

void ProblemDialog::toggleQuestionVisibility()
{
    questionCollapsed = !questionCollapsed;
    if (questionCollapsed) {
        expandedSize = size();
    }

    contentContainer->setVisible(!questionCollapsed);
    if (buttonBox) {
        buttonBox->setVisible(!questionCollapsed);
    }

    if (questionCollapsed) {
        toggleQuestionButton->setArrowType(Qt::DownArrow);
        toggleQuestionButton->setText(tr("Pregunta oculta — clic para mostrar"));
        toggleQuestionButton->setToolTip(QString());

        const int minHeaderHeight = 40;
        const int padding = 16;
        const int extraBottomSpace = 12;
        const int headerHeight = qMax(minHeaderHeight, toggleQuestionButton->sizeHint().height() + padding);
        collapsedHeight = headerHeight + extraBottomSpace;
        toggleQuestionButton->setMinimumHeight(headerHeight - 8);

        setSizeGripEnabled(false);
        setMinimumSize(expandedSize.width(), collapsedHeight);
        setMaximumSize(expandedSize.width(), collapsedHeight);
        resize(expandedSize.width(), collapsedHeight);
    } else {
        toggleQuestionButton->setArrowType(Qt::UpArrow);
        toggleQuestionButton->setText(tr("Ocultar pregunta"));
        toggleQuestionButton->setToolTip(QString());
        toggleQuestionButton->setMinimumSize(0, 0);
        toggleQuestionButton->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        toggleQuestionButton->setMinimumHeight(0);

        setMinimumSize(260, 160);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setSizeGripEnabled(true);
        if (expandedSize.isValid()) {
            resize(expandedSize);
        } else {
            adjustSize();
        }
    }
}
