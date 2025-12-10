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
#include <QMenu>
#include <QKeyEvent>
#include <QEvent>
#include <QPoint>
#include <QMouseEvent>
#include <QPointF>
#include <QStringList>
#include <QSignalBlocker>
#include <QColorDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      scene(new QGraphicsScene(this)),
      view(new QGraphicsView(this)),
      dibujos(scene, view),
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
    // Punto
    ui->actiondibujar_punto->setCheckable(true);
    connect(ui->actiondibujar_punto, &QAction::toggled,
            this, &MainWindow::setDrawPointMode);
    // Linea
    ui->actiondibujar_linea->setCheckable(true);
    connect(ui->actiondibujar_linea, &QAction::toggled,
            this, &MainWindow::setDrawLineMode);


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
    ui->actiontransportador->setChecked(false); // empiezan ocultos

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
    const QColor chosen = QColorDialog::getColor(dibujos.lineColor(),
                                                 this,
                                                 tr("Selecciona un color"));
    if (!chosen.isValid()) {
        return;
    }

    dibujos.setLineColor(chosen);
    dibujos.setPointColor(chosen);
}

void MainWindow::on_actionreset_triggered()
{
    dibujos.reset();

    {
        const QSignalBlocker blocker(ui->actiondibujar_linea);
        ui->actiondibujar_linea->setChecked(false);
    }
    {
        const QSignalBlocker blocker(ui->actiondibujar_punto);
        ui->actiondibujar_punto->setChecked(false);
    }
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

void MainWindow::setDrawLineMode(bool enabled)
{
    dibujos.setDrawLineMode(enabled);

    if (!dibujos.drawPointMode() && ui->actiondibujar_punto->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_punto);
        ui->actiondibujar_punto->setChecked(false);
    }

    const QSignalBlocker blocker(ui->actiondibujar_linea);
    ui->actiondibujar_linea->setChecked(dibujos.drawLineMode());
}

void MainWindow::setDrawPointMode(bool enabled)
{
    dibujos.setDrawPointMode(enabled);

    if (!dibujos.drawLineMode() && ui->actiondibujar_linea->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_linea);
        ui->actiondibujar_linea->setChecked(false);
    }

    const QSignalBlocker blocker(ui->actiondibujar_punto);
    ui->actiondibujar_punto->setChecked(dibujos.drawPointMode());
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (dibujos.handleEvent(obj, event)) {
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        bool handled = false;
        if (dibujos.drawLineMode()) {
            setDrawLineMode(false);
            handled = true;
        }
        if (dibujos.drawPointMode()) {
            setDrawPointMode(false);
            handled = true;
        }
        if (handled) {
            // Salir del modo dibujo si esta activo
            event->accept();
            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::on_actionpuntos_mapa_triggered()
{
    const auto &coords = dibujos.pointCoordinates();
    if (coords.isEmpty()) {
        QMessageBox::information(this, tr("Puntos"), tr("No hay puntos guardados."));
        return;
    }

    QStringList lines;
    for (int i = 0; i < coords.size(); ++i) {
        const QPointF &p = coords.at(i);
        lines << tr("Punto %1: X=%2, Y=%3")
                 .arg(i + 1)
                 .arg(p.x(), 0, 'f', 1)
                 .arg(p.y(), 0, 'f', 1);
    }

    QMessageBox::information(this,
                             tr("Puntos en el mapa"),
                             lines.join("\n"));
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
