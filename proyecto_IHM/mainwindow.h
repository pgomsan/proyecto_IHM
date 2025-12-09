#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:


    void on_actionzoom_in_triggered();

    void on_actionzoom_out_triggered();

    void on_actionmenu_usuario_triggered();

    void handleLoginRequested(const QString &username, const QString &password);
    void handleRegisterRequested();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QGraphicsView *view;
    void applyZoom();
    double currentZoom = 0.2;
    static constexpr double kMinZoom = 0.09;
    static constexpr double kMaxZoom = 1.0;
};
#endif // MAINWINDOW_H
