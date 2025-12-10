#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>
#include <QPoint>
#include "useragent.h"
#include "profiledialog.h"
#include "tool.h"
#include "dibujos.h"

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
    void on_actionreset_triggered();


    void setDrawPointMode(bool enabled);
    void setDrawLineMode(bool enabled);

    void handleLoginRequested(const QString &username, const QString &password);
    void handleRegisterRequested();
    void on_actionpuntos_mapa_triggered();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QGraphicsView *view;
    Dibujos dibujos;
    UserAgent userAgent;
    void applyZoom();
    void updateUserActionIcon();
    double currentZoom = 0.2;
    static constexpr double kMinZoom = 0.09;
    static constexpr double kMaxZoom = 1.0;

    // Display de herramientas
    Tool* m_protractor = nullptr;
    Tool* m_ruler = nullptr;
    Tool* m_compass = nullptr;
    void setProtractorVisible(bool visible);
    void setRulerVisible(bool visible);
    void setCompassVisible(bool visible);
    void addPointAt(const QPointF &scenePos);

protected:
    QMenu *createPopupMenu() override; // Para que no se pueda quitar el toolbar
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};


#endif // MAINWINDOW_H
