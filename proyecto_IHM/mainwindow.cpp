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
            QMessageBox::information(this, tr("Sesión"), tr("Has cerrado sesión."));
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
        QMessageBox::warning(this, tr("Inicio de sesión fallido"), error);
        return;
    }
    const User *user = userAgent.currentUser();
    QMessageBox::information(this, tr("Inicio de sesión"),
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


