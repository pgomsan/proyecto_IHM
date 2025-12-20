#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QVector>
#include <QPoint>
#include "navdb/lib/include/navtypes.h"

class QLabel;
class QVBoxLayout;
class QButtonGroup;
class QToolButton;
class QDialogButtonBox;
class QMouseEvent;
class QPushButton;
class QEvent;

class ProblemDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProblemDialog(QWidget *parent = nullptr);

    void setProblem(const Problem &problem);

signals:
    void nextRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void handleCheckAnswer();
    void toggleQuestionVisibility();

private:
    void installDragFilters();
    static bool isInteractiveWidget(QObject *watched);

    void clearAnswers();
    void addAnswerOption(const Answer &answer);

    QLabel *questionLabel = nullptr;
    QVBoxLayout *answersLayout = nullptr;
    QButtonGroup *answerGroup = nullptr;
    QWidget *answersContainer = nullptr;
    class QScrollArea *answersScroll = nullptr;
    QWidget *contentContainer = nullptr;
    QToolButton *toggleQuestionButton = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    QPushButton *checkButton = nullptr;
    QPushButton *nextButton = nullptr;
    bool questionCollapsed = false;
    QSize expandedSize;
    int collapsedHeight = 0;
    bool answerChecked = false;

    bool m_draggingWindow = false;
    QPoint m_dragStartGlobalPos;
    QPoint m_dragStartWindowPos;
};

#endif // QUESTIONDIALOG_H
