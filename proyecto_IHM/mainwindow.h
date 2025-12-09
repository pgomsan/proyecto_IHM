#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>
#include "useragent.h"
#include "profiledialog.h"

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

    // Zoom
    void on_actionzoom_in_triggered();
    void on_actionzoom_out_triggered();
    void on_actionmenu_usuario_triggered();




    void setDrawLineMode(bool enabled);

    void handleLoginRequested(const QString &username, const QString &password);
    void handleRegisterRequested();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QGraphicsView *view;
    UserAgent userAgent;
    void applyZoom();
    void updateUserActionIcon();
    double currentZoom = 0.2;
    static constexpr double kMinZoom = 0.09;
    static constexpr double kMaxZoom = 1.0;

    // Dibujo de lineas
    bool m_drawLineMode = false;
    QGraphicsLineItem *m_currentLineItem = nullptr;
    QPointF m_lineStart;

protected:
    QMenu *createPopupMenu() override; // Para que no se pueda quitar el toolbar
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};


#endif // MAINWINDOW_H
