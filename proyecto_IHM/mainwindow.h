#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVector>
#include <QColor>
#include <QPoint>
#include <QSize>
#include "useragent.h"
#include "profiledialog.h"
#include "tool.h"
#include "dibujos.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QGraphicsScene;
class QGraphicsView;
class QMenu;
class QGraphicsProxyWidget;
class QTextEdit;
class QWidget;

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
    void on_actionhistorial_triggered();
    void on_actionreset_triggered();
    void on_actioncolores_triggered();
    void on_actionMiMenu_preguntas_triggered();
    void on_actionPregunta_aleatoria_triggered();
    void on_actionpuntos_mapa_toggled(bool checked);
    void showRandomProblem();
    void openQuestionBank();
    void openProblemDialog(const Problem &problem);
    void setAddTextMode(bool enabled);
    void onSceneSelectionChanged();


    void setDrawPointMode(bool enabled);
    void setDrawLineMode(bool enabled);
    void setDrawArcMode(bool enabled);

    void handleLoginRequested(const QString &username, const QString &password);
    void handleRegisterRequested();


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

    // Dibujo de lineas
    bool m_eraserMode = false;

    // Display de herramientas
    Tool* m_protractor = nullptr;
    Tool* m_ruler = nullptr;
    Tool* m_compass = nullptr;
    void setProtractorVisible(bool visible);
    void setRulerVisible(bool visible);
    void setCompassVisible(bool visible);
    void setEraserMode(bool enabled);
    void clearTextBoxes();
    void selectTextBox(QGraphicsProxyWidget *proxy);
    QGraphicsProxyWidget *createTextBoxAt(const QPointF &scenePos);
    struct TextBoxWidgets {
        QGraphicsProxyWidget *proxy = nullptr;
        QWidget *container = nullptr;
        QTextEdit *editor = nullptr;
        bool resizing = false;
        QPoint resizeStartPos;
        QSize resizeStartSize;
        double resizeStartFontSize = 16.0;
    };
    QVector<TextBoxWidgets> m_textBoxes;
    QGraphicsProxyWidget *m_activeTextBox = nullptr;
    QColor m_textColor = Qt::black;
    bool m_addTextMode = false;
    bool m_rightDragInProgress = false;
    bool m_textPlacementPending = false;
    QPoint m_lastRightDragPos;
    TextBoxWidgets *findTextBox(QGraphicsProxyWidget *proxy);
    TextBoxWidgets *findTextBox(QWidget *container);
    void applyColorToActiveText(const QColor &color);
    void autoResizeTextBox(TextBoxWidgets *box);
    void markAddTextInactive();
    void showPointPopups();
    void clearPointPopups();
    void removePointPopup(QGraphicsProxyWidget *popup);
    void refreshPointPopups();
    QVector<QGraphicsProxyWidget*> m_pointPopups;

protected:
    QMenu *createPopupMenu() override; // Para que no se pueda quitar el toolbar
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};


#endif // MAINWINDOW_H
