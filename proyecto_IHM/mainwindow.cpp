#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "profiledialog.h"
#include "questiondialog.h"
#include "questionbankdialog.h"
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
#include <QGraphicsItem>
#include <QPointF>
#include <QMenu>
#include <QKeyEvent>
#include <QEvent>
#include <QPoint>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QSignalBlocker>
#include <QColorDialog>
#include <QGraphicsProxyWidget>
#include <QToolButton>
#include <QLabel>

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
    ui->actiontransportador->setChecked(false); // empiezan ocultos

    ui->actionpuntos_mapa->setCheckable(true);
    connect(ui->actionpuntos_mapa, &QAction::toggled,
            this, &MainWindow::on_actionpuntos_mapa_toggled);

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

void MainWindow::on_actionpuntos_mapa_toggled(bool checked)
{
    if (checked) {
        showPointPopups();
    } else {
        clearPointPopups();
    }
}

void MainWindow::on_actionreset_triggered()
{
    dibujos.reset();
    ui->actionpuntos_mapa->setChecked(false);
    clearPointPopups();

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
    ui->actionPregunta_aleatoria->setEnabled(loggedIn);
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

void MainWindow::on_actionMiMenu_preguntas_triggered()
{
    openQuestionBank();
}

void MainWindow::on_actionPregunta_aleatoria_triggered()
{
    showRandomProblem();
}

void MainWindow::showRandomProblem()
{
    const auto &problems = Navigation::instance().problems();
    if (problems.isEmpty()) {
        QMessageBox::information(this, tr("Preguntas"),
                                 tr("No hay preguntas disponibles en la base de datos."));
        return;
    }
    const int idx = QRandomGenerator::global()->bounded(problems.size());
    openProblemDialog(problems.at(idx));
}

void MainWindow::openQuestionBank()
{
    const auto &problems = Navigation::instance().problems();
    if (problems.isEmpty()) {
        QMessageBox::information(this, tr("Preguntas"),
                                 tr("No hay preguntas disponibles en la base de datos."));
        return;
    }

    QuestionBankDialog dialog(problems, this);
    connect(&dialog, &QuestionBankDialog::problemSelected,
            this, &MainWindow::openProblemDialog);
    dialog.exec();
}

void MainWindow::openProblemDialog(const Problem &problem)
{
    auto *dialog = new ProblemDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setProblem(problem);
    dialog->show();
}

void MainWindow::showPointPopups()
{
    clearPointPopups();

    const auto &points = dibujos.pointCoordinates();
    for (int i = 0; i < points.size(); ++i) {
        const QPointF &scenePos = points.at(i);
        auto geo = dibujos.screenToGeo(scenePos.x(), scenePos.y());
        const QString latStr = dibujos.formatDMS(geo.first, true);
        const QString lonStr = dibujos.formatDMS(geo.second, false);

        QWidget *card = new QWidget;
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setStyleSheet("background: rgba(255, 255, 255, 0.95);"
                            "border: 1px solid #2d2d2d;"
                            "border-radius: 8px;");

        auto *layout = new QHBoxLayout(card);
        layout->setContentsMargins(10, 8, 8, 8);
        layout->setSpacing(10);

        auto *label = new QLabel(tr("Punto %1\nLatitud: %2\nLongitud: %3")
                                 .arg(i + 1)
                                 .arg(latStr)
                                 .arg(lonStr),
                                 card);
        label->setWordWrap(true);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setStyleSheet("color: #1f1f1f; font-size: 65px; font-weight: 700; line-height: 1.3;");

        auto *closeButton = new QToolButton(card);
        closeButton->setText(QStringLiteral("x"));
        closeButton->setToolTip(tr("Cerrar este punto"));
        closeButton->setAutoRaise(true);
        closeButton->setStyleSheet("color: #c00000; font-size: 28px; font-weight: 800;");

        layout->addWidget(label);
        layout->addWidget(closeButton, 0, Qt::AlignTop);

        auto *proxy = scene->addWidget(card);
        proxy->setZValue(30);
        const QSize hint = card->sizeHint();
        card->setMinimumSize(hint.width() , hint.height());
        proxy->setFlag(QGraphicsItem::ItemIsSelectable, true);
        proxy->setPos(scenePos + QPointF(20.0, -70.0));

        connect(closeButton, &QToolButton::clicked, this, [this, proxy]() {
            removePointPopup(proxy);
        });

        m_pointPopups.append(proxy);
    }
}

void MainWindow::clearPointPopups()
{
    for (QGraphicsProxyWidget *popup : m_pointPopups) {
        if (!popup) {
            continue;
        }
        scene->removeItem(popup);
        popup->deleteLater();
    }
    m_pointPopups.clear();
}

void MainWindow::removePointPopup(QGraphicsProxyWidget *popup)
{
    if (!popup) {
        return;
    }

    scene->removeItem(popup);
    m_pointPopups.removeOne(popup);
    popup->deleteLater();

    if (m_pointPopups.isEmpty() && ui->actionpuntos_mapa->isChecked()) {
        ui->actionpuntos_mapa->blockSignals(true);
        ui->actionpuntos_mapa->setChecked(false);
        ui->actionpuntos_mapa->blockSignals(false);
    }
}

void MainWindow::refreshPointPopups()
{
    if (ui->actionpuntos_mapa->isChecked()) {
        showPointPopups();
    }
}

// Dibujo de lineas con click derecho
void MainWindow::setDrawLineMode(bool enabled)
{
    if (enabled && m_eraserMode && ui->actionborrador->isChecked()) {
        ui->actionborrador->setChecked(false);
    }

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
    if (enabled && m_eraserMode && ui->actionborrador->isChecked()) {
        ui->actionborrador->setChecked(false);
    }

    dibujos.setDrawPointMode(enabled);

    if (!dibujos.drawLineMode() && ui->actiondibujar_linea->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_linea);
        ui->actiondibujar_linea->setChecked(false);
    }

    const QSignalBlocker blocker(ui->actiondibujar_punto);
    ui->actiondibujar_punto->setChecked(dibujos.drawPointMode());
}

void MainWindow::setEraserMode(bool enabled)
{
    m_eraserMode = enabled;

    if (m_eraserMode) {
        // Desactivar modos de dibujo para evitar conflictos
        if (ui->actiondibujar_linea->isChecked()) {
            const QSignalBlocker blocker(ui->actiondibujar_linea);
            ui->actiondibujar_linea->setChecked(false);
        }
        if (ui->actiondibujar_punto->isChecked()) {
            const QSignalBlocker blocker(ui->actiondibujar_punto);
            ui->actiondibujar_punto->setChecked(false);
        }
        dibujos.setDrawLineMode(false);
        dibujos.setDrawPointMode(false);

        // Mantener el desplazamiento con click izquierdo, borrar con click derecho
        view->setDragMode(QGraphicsView::ScrollHandDrag);

    } else {
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->unsetCursor();
    }
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    const int previousPointCount = dibujos.pointCoordinates().size();

    if (obj == view->viewport()) {
        if (m_eraserMode) {
            auto *e = static_cast<QMouseEvent*>(event);
            const bool rightPress =
                (event->type() == QEvent::MouseButtonPress && e->button() == Qt::RightButton);
            const bool rightDrag  =
                (event->type() == QEvent::MouseMove && (e->buttons() & Qt::RightButton));

            if (rightPress || rightDrag) {
                const QPointF scenePos = view->mapToScene(e->pos());
                const QList<QGraphicsItem*> hitItems = scene->items(
                    scenePos,
                    Qt::IntersectsItemShape,
                    Qt::DescendingOrder,
                    view->transform());

                for (QGraphicsItem *hitItem : hitItems) {
                    if (dibujos.eraseLineItem(hitItem)) {
                        refreshPointPopups();
                        return true;
                    }
                    if (dibujos.erasePointItem(hitItem)) {
                        refreshPointPopups();
                        return true;
                    }
                }
            }
        }
    }

    const bool handled = dibujos.handleEvent(obj, event);

    if (ui->actionpuntos_mapa->isChecked() &&
            dibujos.pointCoordinates().size() != previousPointCount) {
        showPointPopups();
    }

    if (handled) {
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
        if (m_eraserMode && ui->actionborrador->isChecked()) {
            ui->actionborrador->setChecked(false);
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
