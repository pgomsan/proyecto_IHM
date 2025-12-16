#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QVector>
#include "navdb/lib/include/navtypes.h"

class QLabel;
class QVBoxLayout;
class QButtonGroup;
class QToolButton;
class QDialogButtonBox;
class QMouseEvent;

class ProblemDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProblemDialog(QWidget *parent = nullptr);

    void setProblem(const Problem &problem);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void handleCheckAnswer();
    void toggleQuestionVisibility();

private:
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
    bool questionCollapsed = false;
    QSize expandedSize;
    int collapsedHeight = 0;
};

#endif // QUESTIONDIALOG_H
