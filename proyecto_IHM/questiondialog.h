#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QVector>
#include "navdb/lib/include/navtypes.h"

class QLabel;
class QVBoxLayout;
class QButtonGroup;

class ProblemDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProblemDialog(QWidget *parent = nullptr);

    void setProblem(const Problem &problem);

private slots:
    void handleCheckAnswer();

private:
    void clearAnswers();
    void addAnswerOption(const Answer &answer);

    QLabel *questionLabel = nullptr;
    QVBoxLayout *answersLayout = nullptr;
    QButtonGroup *answerGroup = nullptr;
    QWidget *answersContainer = nullptr;
    class QScrollArea *answersScroll = nullptr;
};

#endif // QUESTIONDIALOG_H
