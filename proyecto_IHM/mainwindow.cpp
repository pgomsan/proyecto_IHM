#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "profiledialog.h"
#include "questiondialog.h"
#include "questionbankdialog.h"
#include "historydialog.h"
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
#include <QTextEdit>
#include <QScrollBar>
#include <QTextCursor>
#include <QFrame>
#include <QSizePolicy>
#include <QTextCharFormat>
#include <QFontMetrics>
#include <QCursor>
#include <QPointer>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <cmath>
#include <algorithm>

namespace {
class ConstrainedTextProxyWidget final : public QGraphicsProxyWidget
{
public:
    explicit ConstrainedTextProxyWidget(const QRectF &constraintRect)
        : m_constraintRect(constraintRect)
    {
    }

    void setConstraintRect(const QRectF &rect)
    {
        m_constraintRect = rect;
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == QGraphicsItem::ItemPositionChange && !m_constraintRect.isNull()) {
            QPointF newPos = value.toPointF();
            const QRectF bounds = boundingRect();
            QRectF allowed = m_constraintRect.adjusted(0.0, 0.0, -bounds.width(), -bounds.height());
            if (allowed.width() < 0.0) {
                allowed.setLeft(m_constraintRect.left());
                allowed.setRight(m_constraintRect.left());
            }
            if (allowed.height() < 0.0) {
                allowed.setTop(m_constraintRect.top());
                allowed.setBottom(m_constraintRect.top());
            }
            newPos.setX(std::clamp(newPos.x(), allowed.left(), allowed.right()));
            newPos.setY(std::clamp(newPos.y(), allowed.top(), allowed.bottom()));
            return newPos;
        }
        return QGraphicsProxyWidget::itemChange(change, value);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            const bool moveWithModifier =
                (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier));

            const QRectF r = boundingRect();
            const qreal border = 14.0;
            const QRectF resizeHandleRect(r.right() - 18.0, r.bottom() - 18.0, 18.0, 18.0);
            const bool isResizeHandle = resizeHandleRect.contains(event->pos());
            const bool inBorder =
                (event->pos().x() <= r.left() + border) ||
                (event->pos().x() >= r.right() - border) ||
                (event->pos().y() <= r.top() + border) ||
                (event->pos().y() >= r.bottom() - border);

            if ((moveWithModifier || inBorder) && !isResizeHandle) {
                setSelected(true);
                m_dragging = true;
                m_pressScenePos = event->scenePos();
                m_startPos = pos();
                setCursor(Qt::SizeAllCursor);
                event->accept();
                return;
            }
        }

        QGraphicsProxyWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_dragging) {
            const QPointF delta = event->scenePos() - m_pressScenePos;
            setPos(m_startPos + delta);
            event->accept();
            return;
        }

        QGraphicsProxyWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_dragging && event->button() == Qt::LeftButton) {
            m_dragging = false;
            unsetCursor();
            event->accept();
            return;
        }

        QGraphicsProxyWidget::mouseReleaseEvent(event);
    }

private:
    QRectF m_constraintRect;
    bool m_dragging = false;
    QPointF m_pressScenePos;
    QPointF m_startPos;
};
}

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
    view->setDragMode(QGraphicsView::NoDrag);
    view->setFrameShape(QFrame::NoFrame);
    view->setFocusPolicy(Qt::StrongFocus);
    view->viewport()->setFocusPolicy(Qt::StrongFocus);
    view->setMouseTracking(true);
    view->viewport()->setMouseTracking(true);

    auto *mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);
    mainLayout->addWidget(view, 1);

    QPixmap pm(":/images/carta_nautica.jpg");
    QGraphicsPixmapItem *item = scene->addPixmap(pm);
    item->setZValue(0);
    m_chartRect = item->boundingRect();
    scene->setSceneRect(m_chartRect);

    currentZoom = 0.20;
    applyZoom();
    updateUserActionIcon();

    // Configuracion de la Tool Bar
    ui->toolBar->setIconSize(QSize(56, 56));
    ui->toolBar->setMovable(false);
    ui->toolBar->setFloatable(false);
    ui->actioncerrar_sesion->setText(tr("Cerrar sesión"));
    ui->actioncerrar_sesion->setToolTip(tr("Cerrar sesión"));
    ui->actioncerrar_sesion->setIcon(QIcon(":/icons/logout.svg"));
    ui->actioncerrar_sesion->setIconVisibleInMenu(false);

    auto *toolBarSpacer = new QWidget(ui->toolBar);
    toolBarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_logoutSpacerAction = ui->toolBar->addWidget(toolBarSpacer);

    m_logoutButton = new QToolButton(ui->toolBar);
    m_logoutButton->setAutoRaise(true);
    m_logoutButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_logoutButton->setIconSize(QSize(40, 40));
    m_logoutButton->setDefaultAction(ui->actioncerrar_sesion);
    m_logoutButtonAction = ui->toolBar->addWidget(m_logoutButton);

    auto *logoutRightPad = new QWidget(ui->toolBar);
    logoutRightPad->setFixedWidth(14);
    m_logoutRightPadAction = ui->toolBar->addWidget(logoutRightPad);

    updateUserActionIcon();

    // Configuracion de dibujos
    // Punto
    ui->actiondibujar_punto->setCheckable(true);
    connect(ui->actiondibujar_punto, &QAction::toggled,
            this, &MainWindow::setDrawPointMode);
    // Linea
    ui->actiondibujar_linea->setCheckable(true);
    connect(ui->actiondibujar_linea, &QAction::toggled,
            this, &MainWindow::setDrawLineMode);
    // Curva
    ui->actiondibujar_curva->setCheckable(true);
    connect(ui->actiondibujar_curva, &QAction::toggled,
            this, &MainWindow::setDrawArcMode);

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

    ui->actionanadir_texto->setCheckable(true);
    connect(ui->actionanadir_texto, &QAction::toggled,
            this, &MainWindow::setAddTextMode);

    connect(scene, &QGraphicsScene::selectionChanged,
            this, &MainWindow::onSceneSelectionChanged);

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
    m_textColor = chosen;
    applyColorToActiveText(chosen);
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
    clearTextBoxes();
    markAddTextInactive();
    ui->actionpuntos_mapa->setChecked(false);
    clearPointPopups();
    updateViewCursor();

    {
        const QSignalBlocker blocker(ui->actiondibujar_linea);
        ui->actiondibujar_linea->setChecked(false);
    }
    {
        const QSignalBlocker blocker(ui->actiondibujar_punto);
        ui->actiondibujar_punto->setChecked(false);
    }
    {
        const QSignalBlocker blocker(ui->actiondibujar_curva);
        ui->actiondibujar_curva->setChecked(false);
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
    ui->actioncerrar_sesion->setEnabled(loggedIn);
    ui->actioncerrar_sesion->setVisible(loggedIn);
    if (m_logoutSpacerAction) {
        m_logoutSpacerAction->setVisible(loggedIn);
    }
    if (m_logoutButtonAction) {
        m_logoutButtonAction->setVisible(loggedIn);
    }
    if (m_logoutRightPadAction) {
        m_logoutRightPadAction->setVisible(loggedIn);
    }
    if (m_logoutButton) {
        m_logoutButton->setVisible(loggedIn);
    }
}

//Registro de usuario
void MainWindow::on_actionmenu_usuario_triggered()
{
    if (userAgent.isLoggedIn()) {
        ProfileDialog dialog(this);
        dialog.setUser(userAgent.currentUser());
        connect(&dialog, &ProfileDialog::profileUpdated, this,
                [this](const QString &password, const QString &email, const QDate &birthdate, const QImage &avatar) {
            auto &nav = Navigation::instance();
            const User *current = userAgent.currentUser();
            if (!current) {
                return;
            }
            User updated = *current;
            updated.setPassword(password);
            updated.setEmail(email);
            updated.setBirthdate(birthdate);
            updated.setAvatar(avatar);
            try {
                nav.updateUser(updated);
            } catch (const NavDAOException &ex) {
                QMessageBox::critical(this, tr("Error de base de datos"),
                                      tr("No se pudo actualizar el perfil: %1").arg(ex.what()));
            }
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

void MainWindow::on_actionhistorial_triggered()
{
    if (!userAgent.isLoggedIn()) {
        QMessageBox::information(this, tr("Historial"),
                                 tr("Inicia sesión para ver tu historial."));
        return;
    }

    const User *current = userAgent.currentUser();
    if (!current) {
        QMessageBox::warning(this, tr("Historial"),
                             tr("No se pudo cargar el usuario actual."));
        return;
    }

    HistoryDialog dialog(this);
    dialog.setSessions(current->sessions());
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
}
void MainWindow::handleRegisterRequested()
{
    RegisterDialog dialog(this);
    connect(&dialog, &RegisterDialog::registerRequested, this,
            [this](const QString &username, const QString &password,
                   const QString &email, const QDate &birthdate, const QImage &avatar) {
        auto &nav = Navigation::instance();
        if (nav.findUser(username)) {
            QMessageBox::warning(this, tr("Registro"),
                                 tr("El usuario %1 ya existe.").arg(username));
            return;
        }

        try {
            User user(username, email, password, avatar, birthdate);
            nav.addUser(user);
            userAgent.login(username, password, nullptr); // auto-login suave tras registro
            updateUserActionIcon();
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
    QPointer<ProblemDialog> safeDialog(dialog);
    connect(dialog, &ProblemDialog::nextRequested, this, [safeDialog]() {
        if (!safeDialog) {
            return;
        }
        const auto &problems = Navigation::instance().problems();
        if (problems.isEmpty()) {
            return;
        }
        const int idx = QRandomGenerator::global()->bounded(problems.size());
        safeDialog->setProblem(problems.at(idx));
    });
    if (isVisible()) {
        const QPoint topRightGlobal = mapToGlobal(QPoint(width(), 0));
        dialog->move(topRightGlobal.x() - dialog->width() - 12, topRightGlobal.y() + 60);
    }
    dialog->show();
}

void MainWindow::setAddTextMode(bool enabled)
{
    m_addTextMode = enabled;
    m_textPlacementPending = false;
    m_rightDragInProgress = false;
    m_leftPanPressed = false;
    m_leftPanInProgress = false;

    if (enabled) {
        // Desactivar otros modos que usan el click derecho
        if (ui->actiondibujar_linea->isChecked()) {
            const QSignalBlocker blocker(ui->actiondibujar_linea);
            ui->actiondibujar_linea->setChecked(false);
        }
        if (ui->actiondibujar_punto->isChecked()) {
            const QSignalBlocker blocker(ui->actiondibujar_punto);
            ui->actiondibujar_punto->setChecked(false);
        }
        if (ui->actiondibujar_curva->isChecked()) {
            const QSignalBlocker blocker(ui->actiondibujar_curva);
            ui->actiondibujar_curva->setChecked(false);
        }
        dibujos.setDrawLineMode(false);
        dibujos.setDrawPointMode(false);
        dibujos.setDrawArcMode(false);

        if (ui->actionborrador->isChecked()) {
            const QSignalBlocker blocker(ui->actionborrador);
            ui->actionborrador->setChecked(false);
        }
        m_eraserMode = false;

        view->setDragMode(QGraphicsView::NoDrag);
    }

    updateViewCursor();
}

void MainWindow::markAddTextInactive()
{
    if (ui->actionanadir_texto->isChecked()) {
        const QSignalBlocker blocker(ui->actionanadir_texto);
        ui->actionanadir_texto->setChecked(false);
    }
    m_addTextMode = false;
    m_textPlacementPending = false;
    m_rightDragInProgress = false;
    m_leftPanPressed = false;
    m_leftPanInProgress = false;
    updateViewCursor();
}

MainWindow::TextBoxWidgets *MainWindow::findTextBox(QGraphicsProxyWidget *proxy)
{
    if (!proxy) {
        return nullptr;
    }

    for (auto &box : m_textBoxes) {
        if (box.proxy == proxy) {
            return &box;
        }
    }
    return nullptr;
}

MainWindow::TextBoxWidgets *MainWindow::findTextBox(QWidget *container)
{
    if (!container) {
        return nullptr;
    }

    for (auto &box : m_textBoxes) {
        if (box.container == container) {
            return &box;
        }
        if (box.container && box.container->isAncestorOf(container)) {
            return &box;
        }
    }
    return nullptr;
}

bool MainWindow::eraseTextBoxItem(QGraphicsItem *item)
{
    QGraphicsItem *currentItem = item;
    while (currentItem) {
        auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(currentItem);
        if (proxy) {
            for (int i = 0; i < m_textBoxes.size(); ++i) {
                if (m_textBoxes[i].proxy == proxy) {
                    if (m_activeTextBox == proxy) {
                        m_activeTextBox = nullptr;
                    }
                    m_textBoxes.removeAt(i);

                    const QSignalBlocker blocker(scene);
                    scene->removeItem(proxy);
                    proxy->deleteLater();
                    return true;
                }
            }
        }
        currentItem = currentItem->parentItem();
    }

    return false;
}

void MainWindow::selectTextBox(QGraphicsProxyWidget *proxy)
{
    m_activeTextBox = proxy;

    for (auto &box : m_textBoxes) {
        const bool isActive = (box.proxy == proxy);
        const QString borderColor = isActive ? "#0060d9" : "transparent";
        if (box.container) {
            box.container->setStyleSheet(QStringLiteral(
                "background: transparent;"
                "border: 1px dashed %1;"
                "border-radius: 6px;"
                "padding: 4px;")
                .arg(borderColor));
        }
        if (isActive && box.editor) {
            m_textColor = box.textColor;
            QTextCharFormat fmt;
            fmt.setForeground(box.textColor);
            box.editor->mergeCurrentCharFormat(fmt);
            box.editor->setTextColor(box.textColor);
            box.editor->setFocus();
        }
    }
}

void MainWindow::applyColorToActiveText(const QColor &color)
{
    if (!m_activeTextBox || !color.isValid()) {
        return;
    }

    TextBoxWidgets *box = findTextBox(m_activeTextBox);
    if (!box || !box->editor) {
        return;
    }

    box->textColor = color;

    QTextCursor cursor = box->editor->textCursor();
    cursor.select(QTextCursor::Document);

    QTextCharFormat fmt;
    fmt.setForeground(color);
    cursor.mergeCharFormat(fmt);
    box->editor->mergeCurrentCharFormat(fmt);
    cursor.clearSelection();
    box->editor->setTextCursor(cursor);
}

void MainWindow::autoResizeTextBox(TextBoxWidgets *box)
{
    if (!box || box->resizing || !box->container || !box->editor || !box->proxy) {
        return;
    }

    const int currentWidth = box->container->width();
    if (currentWidth <= 0) {
        return;
    }

    const double safeZoom = std::max(currentZoom, 0.01);
    const auto scaled = [safeZoom](int pixelsOnScreen) {
        return static_cast<int>(std::round(pixelsOnScreen / safeZoom));
    };

    const int minWidth = std::max(box->container->minimumWidth(), scaled(260));
    const int maxWidth = [&]() -> int {
        if (!view || !view->viewport()) {
            return std::max(minWidth, scaled(900));
        }
        const int viewportScreenWidth = view->viewport()->width();
        const int maxWidthFromViewport =
            static_cast<int>(std::round(viewportScreenWidth / safeZoom)) - scaled(32);
        return std::max(minWidth, maxWidthFromViewport);
    }();

    const QString plainText = box->editor->toPlainText();
    const bool isSingleLine = !plainText.contains(QLatin1Char('\n'));

    if (isSingleLine) {
        const QFontMetricsF fm(box->editor->font());
        const qreal lineWidth = fm.horizontalAdvance(plainText.isEmpty()
                                                         ? QStringLiteral(" ")
                                                         : plainText);
        const int chromePadding = scaled(56);
        const int desiredWidth = std::clamp(
            static_cast<int>(std::ceil(lineWidth)) + chromePadding,
            minWidth,
            maxWidth);

        if (desiredWidth > currentWidth) {
            box->container->resize(desiredWidth, box->container->height());
            box->proxy->resize(box->container->size());
        }
    }

    // Force document layout to wrap to current viewport width for correct height.
    const int viewportWidth = box->editor->viewport()->width();
    if (viewportWidth > 0) {
        box->editor->document()->setTextWidth(viewportWidth);
    }

    const QMargins margins = box->container->contentsMargins();
    const int layoutPadding = margins.top() + margins.bottom();

    const QSizeF docSize = box->editor->document()->size();
    const int docHeight = static_cast<int>(std::ceil(docSize.height()));

    const int minHeight = std::max(box->container->minimumHeight(), scaled(60));
    const int maxHeight = scaled(360);
    const int targetHeight = std::clamp(docHeight + layoutPadding + scaled(16), minHeight, maxHeight);

    if (box->container->height() == targetHeight) {
        return;
    }

    box->container->resize(currentWidth, targetHeight);
    box->proxy->resize(box->container->size());
}

QGraphicsProxyWidget *MainWindow::createTextBoxAt(const QPointF &scenePos)
{
    const double safeZoom = std::max(currentZoom, 0.01);
    const auto scaled = [safeZoom](int pixelsOnScreen) {
        return static_cast<int>(std::round(pixelsOnScreen / safeZoom));
    };

    auto *container = new QWidget;
    container->setAttribute(Qt::WA_StyledBackground, true);
    container->setAttribute(Qt::WA_TranslucentBackground, true);
    container->setStyleSheet("background: transparent;"
                             "border: 1px dashed transparent;"
                             "border-radius: 6px;"
                             "padding: 4px;");
    container->setMinimumSize(scaled(240), scaled(60));
    container->installEventFilter(this);

    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto *editor = new QTextEdit(container);
    editor->setPlaceholderText(tr("Introduce un texto..."));
    editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editor->setMinimumSize(scaled(220), scaled(40));
    editor->setFrameShape(QFrame::NoFrame);
    editor->setTextColor(m_textColor);
    editor->setStyleSheet("background: transparent;");
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->installEventFilter(this);
    if (editor->viewport()) {
        editor->viewport()->installEventFilter(this);
    }
    {
        QFont font = editor->font();
        font.setPointSizeF(64.0);
        editor->setFont(font);
        editor->document()->setDefaultFont(font);

        QTextCursor cursor(editor->document());
        cursor.select(QTextCursor::Document);
        QTextCharFormat fmt;
        fmt.setFontPointSize(64.0);
        cursor.mergeCharFormat(fmt);
        editor->mergeCurrentCharFormat(fmt);
    }

    layout->addWidget(editor);

    container->resize(scaled(420), scaled(90));

    auto *proxy = new ConstrainedTextProxyWidget(m_chartRect);
    proxy->setWidget(container);
    scene->addItem(proxy);
    proxy->setZValue(35);
    proxy->setFlag(QGraphicsItem::ItemIsMovable, false);
    proxy->setFlag(QGraphicsItem::ItemIsSelectable, true);
    proxy->setFlag(QGraphicsItem::ItemIsFocusable, true);
    proxy->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    proxy->setPos(scenePos);
    proxy->resize(container->size());

    m_textBoxes.append(TextBoxWidgets{
        proxy,
        container,
        editor,
        m_textColor
    });

    {
        QPointer<QGraphicsProxyWidget> proxyPtr(proxy);
        connect(editor->document(), &QTextDocument::contentsChanged, this, [this, proxyPtr]() {
            if (!proxyPtr) {
                return;
            }
            autoResizeTextBox(findTextBox(proxyPtr));
        });
    }

    selectTextBox(proxy);
    editor->setFocus();
    autoResizeTextBox(findTextBox(proxy));
    return proxy;
}

void MainWindow::clearTextBoxes()
{
    for (const auto &box : m_textBoxes) {
        if (box.proxy) {
            scene->removeItem(box.proxy);
            box.proxy->deleteLater();
        }
    }
    m_textBoxes.clear();
    m_activeTextBox = nullptr;
}

void MainWindow::onSceneSelectionChanged()
{
    const auto selectedItems = scene->selectedItems();
    for (QGraphicsItem *item : selectedItems) {
        if (auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item)) {
            if (findTextBox(proxy)) {
                selectTextBox(proxy);
                return;
            }
        }
    }

    selectTextBox(nullptr);
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
    if (enabled && m_addTextMode) {
        markAddTextInactive();
    }
    if (enabled && m_eraserMode && ui->actionborrador->isChecked()) {
        ui->actionborrador->setChecked(false);
    }

    dibujos.setDrawLineMode(enabled);

    if (!dibujos.drawPointMode() && ui->actiondibujar_punto->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_punto);
        ui->actiondibujar_punto->setChecked(false);
    }
    if (!dibujos.drawArcMode() && ui->actiondibujar_curva->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_curva);
        ui->actiondibujar_curva->setChecked(false);
    }

    const QSignalBlocker blocker(ui->actiondibujar_linea);
    ui->actiondibujar_linea->setChecked(dibujos.drawLineMode());
    updateViewCursor();
}

void MainWindow::setDrawPointMode(bool enabled)
{
    if (enabled && m_addTextMode) {
        markAddTextInactive();
    }
    if (enabled && m_eraserMode && ui->actionborrador->isChecked()) {
        ui->actionborrador->setChecked(false);
    }

    dibujos.setDrawPointMode(enabled);

    if (!dibujos.drawLineMode() && ui->actiondibujar_linea->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_linea);
        ui->actiondibujar_linea->setChecked(false);
    }
    if (!dibujos.drawArcMode() && ui->actiondibujar_curva->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_curva);
        ui->actiondibujar_curva->setChecked(false);
    }

    const QSignalBlocker blocker(ui->actiondibujar_punto);
    ui->actiondibujar_punto->setChecked(dibujos.drawPointMode());
    updateViewCursor();
}

void MainWindow::setDrawArcMode(bool enabled)
{
    if (enabled && m_addTextMode) {
        markAddTextInactive();
    }
    if (enabled && m_eraserMode && ui->actionborrador->isChecked()) {
        ui->actionborrador->setChecked(false);
    }

    dibujos.setDrawArcMode(enabled);

    if (!dibujos.drawLineMode() && ui->actiondibujar_linea->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_linea);
        ui->actiondibujar_linea->setChecked(false);
    }
    if (!dibujos.drawPointMode() && ui->actiondibujar_punto->isChecked()) {
        const QSignalBlocker blocker(ui->actiondibujar_punto);
        ui->actiondibujar_punto->setChecked(false);
    }

    const QSignalBlocker blocker(ui->actiondibujar_curva);
    ui->actiondibujar_curva->setChecked(dibujos.drawArcMode());
    updateViewCursor();
}

void MainWindow::setEraserMode(bool enabled)
{
    if (enabled) {
        markAddTextInactive();
    }
    m_eraserMode = enabled;
    m_leftPanPressed = false;
    m_leftPanInProgress = false;

    view->setDragMode(QGraphicsView::NoDrag);

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
        if (ui->actiondibujar_curva->isChecked()) {
            const QSignalBlocker blocker(ui->actiondibujar_curva);
            ui->actiondibujar_curva->setChecked(false);
        }
        dibujos.setDrawLineMode(false);
        dibujos.setDrawPointMode(false);
        dibujos.setDrawArcMode(false);

    }

    updateViewCursor();
}

void MainWindow::updateViewCursor()
{
    if (!view || !view->viewport()) {
        return;
    }

    if (m_leftPanInProgress) {
        view->viewport()->setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (m_addTextMode) {
        view->viewport()->setCursor(Qt::IBeamCursor);
        return;
    }

    if (m_eraserMode) {
        static QCursor eraserCursor = []() {
            constexpr int size = 22;
            QPixmap pix(size, size);
            pix.fill(Qt::transparent);
            QPainter painter(&pix);
            painter.setRenderHint(QPainter::Antialiasing, true);
            QPen pen(Qt::black, 2);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QRectF(4.0, 4.0, size - 8.0, size - 8.0));
            return QCursor(pix, size / 2, size / 2);
        }();
        view->viewport()->setCursor(eraserCursor);
        return;
    }

    if (dibujos.drawLineMode() || dibujos.drawPointMode() || dibujos.drawArcMode()) {
        view->viewport()->setCursor(Qt::CrossCursor);
        return;
    }

    view->viewport()->unsetCursor();
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    const int previousPointCount = dibujos.pointCoordinates().size();

    if (obj == view->viewport()) {
        if (event->type() == QEvent::Leave || event->type() == QEvent::FocusOut) {
            if (m_leftPanPressed || m_leftPanInProgress) {
                m_leftPanPressed = false;
                m_leftPanInProgress = false;
                updateViewCursor();
            }
        }

        if (event->type() == QEvent::MouseButtonPress) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::LeftButton) {
                const QPointF scenePos = view->mapToScene(e->pos());
                const QList<QGraphicsItem*> hitItems = scene->items(
                    scenePos,
                    Qt::IntersectsItemBoundingRect,
                    Qt::DescendingOrder,
                    view->transform());

                bool shouldPan = true;
                for (QGraphicsItem *hitItem : hitItems) {
                    if (!hitItem) {
                        continue;
                    }
                    if (auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(hitItem)) {
                        if (findTextBox(proxy)) {
                            shouldPan = false;
                            break;
                        }
                        shouldPan = false;
                        break;
                    }
                    if (dynamic_cast<Tool*>(hitItem)) {
                        shouldPan = false;
                        break;
                    }
                    if (hitItem->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
                        shouldPan = false;
                        break;
                    }
                }

                if (shouldPan) {
                    m_leftPanPressed = true;
                    m_leftPanInProgress = false;
                    m_leftPanStartPos = e->pos();
                    m_leftPanLastPos = e->pos();
                } else {
                    m_leftPanPressed = false;
                    m_leftPanInProgress = false;
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (m_leftPanPressed && (e->buttons() & Qt::LeftButton)) {
                if (!m_leftPanInProgress) {
                    const QPoint deltaFromStart = e->pos() - m_leftPanStartPos;
                    if (deltaFromStart.manhattanLength() > 2) {
                        m_leftPanInProgress = true;
                        updateViewCursor();
                    }
                }

                if (m_leftPanInProgress) {
                    const QPoint delta = e->pos() - m_leftPanLastPos;
                    view->horizontalScrollBar()->setValue(view->horizontalScrollBar()->value() - delta.x());
                    view->verticalScrollBar()->setValue(view->verticalScrollBar()->value() - delta.y());
                    m_leftPanLastPos = e->pos();
                    return true;
                }
            } else if (m_leftPanPressed || m_leftPanInProgress) {
                m_leftPanPressed = false;
                m_leftPanInProgress = false;
                updateViewCursor();
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::LeftButton) {
                const bool wasPanning = m_leftPanInProgress;
                m_leftPanPressed = false;
                m_leftPanInProgress = false;
                updateViewCursor();
                if (wasPanning) {
                    return true;
                }
            }
        }

        if (m_addTextMode) {
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease) {
                auto *e = static_cast<QMouseEvent*>(event);
                if (event->type() == QEvent::MouseButtonPress &&
                        e->button() == Qt::RightButton) {
                    m_lastRightDragPos = e->pos();
                    m_textPlacementPending = true;
                    m_rightDragInProgress = true;
                    return true;
                }

                if (event->type() == QEvent::MouseMove &&
                        (e->buttons() & Qt::RightButton) &&
                        m_rightDragInProgress) {
                    const QPoint delta = e->pos() - m_lastRightDragPos;
                    view->horizontalScrollBar()->setValue(
                                view->horizontalScrollBar()->value() - delta.x());
                    view->verticalScrollBar()->setValue(
                                view->verticalScrollBar()->value() - delta.y());
                    m_lastRightDragPos = e->pos();
                    if (delta.manhattanLength() > 2) {
                        m_textPlacementPending = false;
                    }
                    return true;
                }

                if (event->type() == QEvent::MouseButtonRelease &&
                        e->button() == Qt::RightButton) {
                    const QPointF scenePos = view->mapToScene(e->pos());
                    if (m_textPlacementPending) {
                        const QList<QGraphicsItem*> hitItems = scene->items(
                            scenePos,
                            Qt::IntersectsItemShape,
                            Qt::DescendingOrder,
                            view->transform());
                        for (QGraphicsItem *hitItem : hitItems) {
                            if (auto *proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(hitItem)) {
                                if (findTextBox(proxy)) {
                                    selectTextBox(proxy);
                                    m_textPlacementPending = false;
                                    m_rightDragInProgress = false;
                                    return true;
                                }
                            }
                        }
                        createTextBoxAt(scenePos);
                    }
                    m_textPlacementPending = false;
                    m_rightDragInProgress = false;
                    return true;
                }
            }
        }

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
                    if (eraseTextBoxItem(hitItem)) {
                        return true;
                    }
                    if (dibujos.eraseArcItem(hitItem, scenePos)) {
                        refreshPointPopups();
                        return true;
                    }
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

    if (auto *widget = qobject_cast<QWidget*>(obj)) {
        if (auto *box = findTextBox(widget)) {
            if (widget == box->container) {
                if (event->type() == QEvent::MouseButtonPress) {
                    auto *e = static_cast<QMouseEvent*>(event);
                    if (e->button() == Qt::LeftButton) {
                        const QRect handleRect(widget->width() - 18,
                                               widget->height() - 18,
                                               18,
                                               18);
                        if (handleRect.contains(e->pos())) {
                            box->resizing = true;
                            box->resizeStartPos = e->pos();
                            box->resizeStartSize = widget->size();
                            if (box->editor) {
                                double startSize = box->editor->fontPointSize();
                                if (startSize <= 0.0) {
                                    startSize = 16.0;
                                }
                                box->resizeStartFontSize = startSize;
                            } else {
                                box->resizeStartFontSize = 16.0;
                            }
                            widget->setCursor(Qt::SizeFDiagCursor);
                            return true;
                        }
                    }
                } else if (event->type() == QEvent::MouseMove) {
                    auto *e = static_cast<QMouseEvent*>(event);
                    const QRect handleRect(widget->width() - 18,
                                           widget->height() - 18,
                                           18,
                                           18);
                    if (box->resizing) {
                        const QPoint delta = e->pos() - box->resizeStartPos;
                        QSize newSize = box->resizeStartSize + QSize(delta.x(), delta.y());
                        newSize.setWidth(std::max(newSize.width(), widget->minimumWidth()));
                        newSize.setHeight(std::max(newSize.height(), widget->minimumHeight()));
                        widget->resize(newSize);
                        if (box->proxy) {
                            box->proxy->resize(newSize);
                        }
                        if (box->editor) {
                            const double ratioW = static_cast<double>(newSize.width()) /
                                    std::max(1, box->resizeStartSize.width());
                            const double ratioH = static_cast<double>(newSize.height()) /
                                    std::max(1, box->resizeStartSize.height());
                            const double ratio = std::min(ratioW, ratioH);
                            double newFontSize = box->resizeStartFontSize * ratio;
                            newFontSize = std::clamp(newFontSize, 8.0, 96.0);

                            QTextCursor cursor = box->editor->textCursor();
                            cursor.select(QTextCursor::Document);
                            QTextCharFormat fmt;
                            fmt.setFontPointSize(newFontSize);
                            cursor.mergeCharFormat(fmt);
                            box->editor->mergeCurrentCharFormat(fmt);
                            cursor.clearSelection();
                            box->editor->setTextCursor(cursor);
                        }
                        return true;
                    } else {
                        widget->setCursor(handleRect.contains(e->pos())
                                          ? Qt::SizeFDiagCursor
                                          : Qt::ArrowCursor);
                    }
                } else if (event->type() == QEvent::MouseButtonRelease) {
                    auto *e = static_cast<QMouseEvent*>(event);
                    if (e->button() == Qt::LeftButton && box->resizing) {
                        box->resizing = false;
                        widget->setCursor(Qt::ArrowCursor);
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
        if (dibujos.drawArcMode()) {
            setDrawArcMode(false);
            handled = true;
        }
        if (m_eraserMode && ui->actionborrador->isChecked()) {
            ui->actionborrador->setChecked(false);
            handled = true;
        }
        if (m_addTextMode && ui->actionanadir_texto->isChecked()) {
            markAddTextInactive();
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

void MainWindow::on_actioncerrar_sesion_triggered()
{
    userAgent.logout();
    updateUserActionIcon();
}
