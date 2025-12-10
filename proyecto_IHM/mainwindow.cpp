#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "profiledialog.h"
#include "navdb/lib/include/navigation.h"
#include "navdb/lib/include/navdaoexception.h"
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDate>
#include <QImage>
#include <QIcon>
#include <algorithm>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QGraphicsItem>
#include <QColorDialog>
#include <QPen>
#include <QPointF>
#include <QMenu>
#include <QKeyEvent>
#include <QEvent>
#include <QPoint>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      scene(new QGraphicsScene(this)),
      view(new QGraphicsView(this)),
      userAgent(Navigation::instance())
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
    updateUserActionIcon();

    // Configuracion de la Tool Bar
    ui->toolBar->setIconSize(QSize(45, 45));
    ui->toolBar->setMovable(false);
    ui->toolBar->setFloatable(false);

    // Configuracion de dibujos
    // Linea
    ui->actiondibujar_linea->setCheckable(true);
    connect(ui->actiondibujar_linea, &QAction::toggled,
            this, &MainWindow::setDrawLineMode);

    // Borrador
    ui->actionborrador->setCheckable(true);
    connect(ui->actionborrador, &QAction::toggled,
            this, &MainWindow::setEraserMode);


    // Botones de herramientas
    ui->actiontransportador->setCheckable(true);
    ui->actionregla->setCheckable(true);
    ui->actioncompas->setCheckable(true);

    // Herramientas
    connect(ui->actiontransportador, &QAction::toggled,
            this, &MainWindow::setProtractorVisible);
    connect(ui->actionregla, &QAction::toggled,
            this, &MainWindow::setRulerVisible);
    connect(ui->actioncompas, &QAction::toggled,
            this, &MainWindow::setCompassVisible);

    ui->actionregla->setChecked(false);
    ui->actioncompas->setChecked(false);
    ui->actiontransportador->setChecked(false); // empieza oculto

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

void MainWindow::on_actioncolores_triggered()
{
    const QColor chosen = QColorDialog::getColor(m_lineColor, this, tr("Selecciona un color"));
    if (!chosen.isValid()) {
        return;
    }

    m_lineColor = chosen;

    // Si hay una linea en progreso, actualizamos su color tambien
    if (m_currentLineItem) {
        QPen pen = m_currentLineItem->pen();
        pen.setColor(m_lineColor);
        m_currentLineItem->setPen(pen);
    }
}

void MainWindow::on_actionreset_triggered()
{
    setDrawLineMode(false);

    const auto itemsCopy = scene->items();
    for (QGraphicsItem *item : itemsCopy) {
        if (qgraphicsitem_cast<QGraphicsLineItem*>(item)) {
            scene->removeItem(item);
            delete item;
        }
    }
    m_currentLineItem = nullptr;
}

// Update del icono del usuario
void MainWindow::updateUserActionIcon()
{
    const bool loggedIn = userAgent.isLoggedIn();
    const char *iconPath = loggedIn
            ? ":/icons/user-silhouette.png"
            : ":/icons/user-silhouette-question.png";
    ui->actionmenu_usuario->setIcon(QIcon(QString::fromUtf8(iconPath)));
    ui->actionhistorial->setEnabled(loggedIn);
    ui->actionMiMenu_preguntas->setEnabled(loggedIn);
}
//Registro de usuario
void MainWindow::on_actionmenu_usuario_triggered()
{
    if (userAgent.isLoggedIn()) {
        ProfileDialog dialog(this);
        dialog.setUser(userAgent.currentUser());
        connect(&dialog, &ProfileDialog::profileUpdated, this,
                [this](const QString &password, const QString &email, const QDate &birthdate) {
            auto &nav = Navigation::instance();
            const User *current = userAgent.currentUser();
            if (!current) {
                return;
            }
            User updated = *current;
            updated.setPassword(password);
            updated.setEmail(email);
            updated.setBirthdate(birthdate);
            try {
                nav.updateUser(updated);
                QMessageBox::information(this, tr("Perfil"), tr("Perfil actualizado."));
            } catch (const NavDAOException &ex) {
                QMessageBox::critical(this, tr("Error de base de datos"),
                                      tr("No se pudo actualizar el perfil: %1").arg(ex.what()));
            }
        });
        connect(&dialog, &ProfileDialog::logoutRequested, this, [this]() {
            userAgent.logout();
            updateUserActionIcon();
            QMessageBox::information(this, tr("Sesion"), tr("Has cerrado sesion."));
        });
        dialog.exec();
        return;
    }

    LoginDialog dialog(this);
    connect(&dialog, &LoginDialog::loginRequested,
            this, &MainWindow::handleLoginRequested);
    connect(&dialog, &LoginDialog::registerRequested,
            this, &MainWindow::handleRegisterRequested);
    dialog.exec();
}
void MainWindow::handleLoginRequested(const QString &username, const QString &password)
{
    QString error;
    if (!userAgent.login(username, password, &error)) {
        QMessageBox::warning(this, tr("Inicio de sesion fallido"), error);
        return;
    }
    updateUserActionIcon();
    const User *user = userAgent.currentUser();
    QMessageBox::information(this, tr("Inicio de sesion"),
                             tr("Bienvenido, %1").arg(user ? user->nickName() : username));
}
void MainWindow::handleRegisterRequested()
{
    RegisterDialog dialog(this);
    connect(&dialog, &RegisterDialog::registerRequested, this,
            [this](const QString &username, const QString &password,
                   const QString &email, const QDate &birthdate) {
        auto &nav = Navigation::instance();
        if (nav.findUser(username)) {
            QMessageBox::warning(this, tr("Registro"),
                                 tr("El usuario %1 ya existe.").arg(username));
            return;
        }

        try {
            QImage emptyAvatar;
            User user(username, email, password, emptyAvatar, birthdate);
            nav.addUser(user);
            userAgent.login(username, password, nullptr); // auto-login suave tras registro
            updateUserActionIcon();
            QMessageBox::information(this, tr("Registro"),
                                     tr("Usuario %1 creado.").arg(username));
        } catch (const NavDAOException &ex) {
            QMessageBox::critical(this, tr("Error de base de datos"),
                                  tr("No se pudo registrar: %1").arg(ex.what()));
        }
    });
    dialog.exec();
}

// Dibujo de lineas con click derecho
void MainWindow::setDrawLineMode(bool enabled)
{
    m_drawLineMode = enabled;

    if (m_drawLineMode) {
        if (m_eraserMode && ui->actionborrador->isChecked()) {
            ui->actionborrador->setChecked(false);
        }
        view->setDragMode(QGraphicsView::NoDrag);
        view->setCursor(Qt::CrossCursor);
    } else {
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        if (!m_eraserMode) {
            view->unsetCursor();
        }

        if (m_currentLineItem) {
            scene->removeItem(m_currentLineItem);
            delete m_currentLineItem;
            m_currentLineItem = nullptr;
        }
        if (ui->actiondibujar_linea->isChecked())
            ui->actiondibujar_linea->setChecked(false);
    }
}

void MainWindow::setEraserMode(bool enabled)
{
    m_eraserMode = enabled;

    if (m_eraserMode) {
        if (m_drawLineMode && ui->actiondibujar_linea->isChecked()) {
            ui->actiondibujar_linea->setChecked(false);
        }
        // Mantener el desplazamiento con click izquierdo, borrar con click derecho
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->setCursor(Qt::PointingHandCursor);
    } else {
        if (!m_drawLineMode) {
            view->setDragMode(QGraphicsView::ScrollHandDrag);
            view->unsetCursor();
        }
    }
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view->viewport()) {
        if (m_drawLineMode) {
            // Solo interceptamos eventos cuando estamos en modo dibujar linea
            if (event->type() == QEvent::MouseButtonPress) {
                auto *e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::RightButton) {
                    // Punto inicial de la linea en coordenadas de escena
                    m_lineStart = view->mapToScene(e->pos());

                    QPen pen(m_lineColor, 8);
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
                    // Si la linea es casi un punto, la podemos eliminar
                    QLineF line = m_currentLineItem->line();
                    if (line.length() < 2.0) {
                        scene->removeItem(m_currentLineItem);
                        delete m_currentLineItem;
                    }
                    m_currentLineItem = nullptr;
                    return true;
                }
            }
        } else if (m_eraserMode) {
            auto *e = static_cast<QMouseEvent*>(event);
            const bool rightPress = (event->type() == QEvent::MouseButtonPress && e->button() == Qt::RightButton);
            const bool rightDrag  = (event->type() == QEvent::MouseMove && (e->buttons() & Qt::RightButton));

            if (rightPress || rightDrag) {
                const QPointF scenePos = view->mapToScene(e->pos());
                const QList<QGraphicsItem*> hitItems = scene->items(
                            scenePos,
                            Qt::IntersectsItemShape,
                            Qt::DescendingOrder,
                            view->transform());

                for (QGraphicsItem *hitItem : hitItems) {
                    if (auto *line = qgraphicsitem_cast<QGraphicsLineItem*>(hitItem)) {
                        scene->removeItem(line);
                        delete line;
                        if (line == m_currentLineItem) {
                            m_currentLineItem = nullptr;
                        }
                        return true;
                    }
                }
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_drawLineMode) {
        // Salir del modo dibujo si esta activo
        setDrawLineMode(false);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && m_eraserMode) {
        ui->actionborrador->setChecked(false);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

// Llamadas a tools.h para mostrar las herramientas
void MainWindow::setProtractorVisible(bool visible)
{
    static const QSizeF kProtractorSize(580.0, 380.0);
    Tool::toggleTool(m_protractor,
                     scene,
                     view,
                     ":/icons/transportador.svg",
                     kProtractorSize,
                     QPoint(20, 20),
                     visible);
}
void MainWindow::setRulerVisible(bool visible)
{
    static const QSizeF kRulerSize(600.0, 100.0);
    Tool::toggleTool(m_ruler,
                     scene,
                     view,
                     ":/icons/ruler.svg",
                     kRulerSize,
                     QPoint(20, 150),
                     visible);
}
void MainWindow::setCompassVisible(bool visible)
{
    static const QSizeF kCompassSize(220.0, 360.0);
    Tool::toggleTool(m_compass,
                     scene,
                     view,
                     ":/icons/compass_leg.svg",
                     kCompassSize,
                     QPoint(20, 280),
                     visible);
}
