#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <algorithm>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QPointF>
#include <QMenu>
#include <QKeyEvent>
#include <QEvent>
#include <QMouseEvent>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      scene(new QGraphicsScene(this)),
      view(new QGraphicsView(this))
{
    ui->setupUi(this);

    setWindowTitle("Carta Nautica"); // simple title placeholder
    view->setScene(scene);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    view->setDragMode(QGraphicsView::ScrollHandDrag);

    auto *mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);
    mainLayout->addWidget(view, 1);

    QPixmap pm(":/images/carta_nautica.jpg");
    QGraphicsPixmapItem *item = scene->addPixmap(pm);
    item->setZValue(0);

    currentZoom = 0.20;
    applyZoom();

    // Configuración de la Tool Bar
    ui->toolBar->setIconSize(QSize(45, 45));
    ui->toolBar->setMovable(false);
    ui->toolBar->setFloatable(false);


    ui->actiondibujar_linea->setCheckable(true);
    connect(ui->actiondibujar_linea, &QAction::toggled,
            this, &MainWindow::setDrawLineMode);


    view->viewport()->installEventFilter(this);


}

MainWindow::~MainWindow()
{
    delete ui;
}

QMenu* MainWindow::createPopupMenu()
{
    return nullptr;
}

// Acciones del zoom
void MainWindow::on_actionzoom_in_triggered()
{
    double newZoom = std::clamp(currentZoom * 1.2, kMinZoom, kMaxZoom);
    if (newZoom != currentZoom) {
        currentZoom = newZoom;
        applyZoom();
    }
}
void MainWindow::on_actionzoom_out_triggered()
{

    double newZoom = std::clamp(currentZoom / 1.2, kMinZoom, kMaxZoom);
    if (newZoom != currentZoom) {
        currentZoom = newZoom;
        applyZoom();
    }
}
void MainWindow::applyZoom()
{
    view->resetTransform();
    view->scale(currentZoom, currentZoom);
}

void MainWindow::on_actionmenu_usuario_triggered()
{
    LoginDialog dialog(this);
    connect(&dialog, &LoginDialog::loginRequested,
            this, &MainWindow::handleLoginRequested);
    connect(&dialog, &LoginDialog::registerRequested,
            this, &MainWindow::handleRegisterRequested);
    dialog.exec();
}
void MainWindow::handleLoginRequested(const QString &username, const QString &password)
{
    Q_UNUSED(password);
    QMessageBox::information(this, tr("Inicio de sesión"),
                             tr("Login solicitado para %1").arg(username));
    // TODO: delegar en UserAgent cuando esté disponible.
}

void MainWindow::handleRegisterRequested()
{
    QMessageBox::information(this, tr("Registro"),
                             tr("Abrir flujo de registro"));
    // TODO: enlazar con flujo de registro real.
}


// Dibujo de lineas con click derecho
void MainWindow::setDrawLineMode(bool enabled)
{
    m_drawLineMode = enabled;

    if (m_drawLineMode) {
        view->setDragMode(QGraphicsView::NoDrag);
        view->setCursor(Qt::CrossCursor);
    } else {
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->unsetCursor();

        if (m_currentLineItem) {
            scene->removeItem(m_currentLineItem);
            delete m_currentLineItem;
            m_currentLineItem = nullptr;
        }
        if (ui->actiondibujar_linea->isChecked())
            ui->actiondibujar_linea->setChecked(false);
    }
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view->viewport() && m_drawLineMode) {
        // Solo interceptamos eventos cuando estamos en modo dibujar línea
        if (event->type() == QEvent::MouseButtonPress) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton) {
                // Punto inicial de la línea en coordenadas de escena
                m_lineStart = view->mapToScene(e->pos());

                QPen pen(Qt::red, 8);
                m_currentLineItem = new QGraphicsLineItem();
                m_currentLineItem->setZValue(10);
                m_currentLineItem->setPen(pen);
                m_currentLineItem->setLine(QLineF(m_lineStart, m_lineStart));
                scene->addItem(m_currentLineItem);

                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->buttons() & Qt::RightButton && m_currentLineItem) {
                QPointF p2 = view->mapToScene(e->pos());
                m_currentLineItem->setLine(QLineF(m_lineStart, p2));
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton && m_currentLineItem) {
                // Si la línea es casi un punto, la podemos eliminar
                QLineF line = m_currentLineItem->line();
                if (line.length() < 2.0) {
                    scene->removeItem(m_currentLineItem);
                    delete m_currentLineItem;
                }
                m_currentLineItem = nullptr;
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_drawLineMode) {
        // Salir del modo dibujo si está activo
        setDrawLineMode(false);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}


