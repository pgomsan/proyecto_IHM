#include "tool.h"

#include <QtMath>
#include <algorithm>
#include <QApplication>
#include <QGuiApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QSvgRenderer>
#include <QTransform>

Tool::Tool(const QString& svgResourcePath, QGraphicsItem* parent)
    : QGraphicsSvgItem(svgResourcePath, parent)
{
    // Flags para poder moverla, seleccionarla y que ignore las transformaciones del view
    setFlags(QGraphicsItem::ItemIsMovable
             | QGraphicsItem::ItemIsSelectable
             | QGraphicsItem::ItemSendsGeometryChanges
             | QGraphicsItem::ItemIgnoresTransformations);

    // Origen de rotacion = centro del SVG
    updateOrigin();

    // Tamano inicial = tamano natural del SVG
    m_targetSizePx = boundingRect().size();
    applyInitialScale();
}

void Tool::setToolSize(const QSizeF& sizePx)
{
    m_targetSizePx = sizePx;
    applyInitialScale();
}

void Tool::setView(QGraphicsView *view)
{
    m_view = view;
}

void Tool::applyInitialScale()
{
    const QRectF br = boundingRect();
    if (br.isEmpty())
        return;

    const double sx = m_targetSizePx.width()  / br.width();
    const double sy = m_targetSizePx.height() / br.height();
    m_uniformScale = std::min(sx, sy);

    setScale(m_uniformScale);
    updateOrigin();

    if (scene())
        scene()->update();
}

void Tool::updateOrigin()
{
    setTransformOriginPoint(boundingRect().center());
}

Tool* Tool::toggleTool(Tool *&tool,
                       QGraphicsScene *scene,
                       QGraphicsView *view,
                       const QString &resourcePath,
                       const QSizeF &defaultSize,
                       const QPoint &initialViewportPos,
                       bool visible)
{
    if (!scene || !view) {
        return tool;
    }

    if (!tool) {
        tool = new Tool(resourcePath);
        scene->addItem(tool);

        if (defaultSize.width() > 0.0 && defaultSize.height() > 0.0) {
            tool->setToolSize(defaultSize);
        }

        tool->setZValue(1000);
        tool->setPos(view->mapToScene(initialViewportPos));
    }

    tool->setView(view);
    tool->setVisible(visible);
    if (visible) {
        tool->setZValue(1000); // ensure on top si se reactiva
        scene->update();
    }

    return tool;
}

void Tool::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // Solo rotar si está pulsado Shift
    if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
        event->ignore();
        return;
    }

    const int delta = event->delta(); // Qt5: 120 por "clic" de rueda
    if (delta == 0) {
        event->ignore();
        return;
    }

    if (!m_view) {
        double deltaDegrees = (delta / 8.0) * 0.1; // ≈ 1.5° por "clic"
        m_angleDeg += deltaDegrees;
        setRotation(m_angleDeg);
        if (scene())
            scene()->update();
        event->accept();
        return;
    }

    const QPointF anchorScenePos = event->scenePos();
    const QPointF anchorViewportPos = m_view->mapFromScene(anchorScenePos);
    const QTransform deviceBefore = deviceTransform(m_view->viewportTransform());
    bool deviceInvertible = false;
    const QTransform invDeviceBefore = deviceBefore.inverted(&deviceInvertible);
    if (!deviceInvertible) {
        event->ignore();
        return;
    }
    const QPointF anchorItemPos = invDeviceBefore.map(anchorViewportPos);

    double deltaDegrees = (delta / 8.0) * 0.1; // ≈ 1.5° por "clic"
    m_angleDeg += deltaDegrees;
    setRotation(m_angleDeg);

    const QTransform deviceAfter = deviceTransform(m_view->viewportTransform());
    const QPointF anchorViewportPosFromItem = deviceAfter.map(anchorItemPos);
    const QPointF deltaViewport = anchorViewportPos - anchorViewportPosFromItem;

    bool invertible = false;
    const QTransform invView = m_view->viewportTransform().inverted(&invertible);
    if (!invertible) {
        event->ignore();
        return;
    }
    const QPointF deltaScene = invView.map(deltaViewport) - invView.map(QPointF(0.0, 0.0));
    setPos(pos() + deltaScene);

    if (scene())
        scene()->update();

    event->accept();
}

void Tool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsSvgItem::mousePressEvent(event);

    if (event && event->button() == Qt::LeftButton && event->isAccepted() && !m_dragCursorActive) {
        QGuiApplication::setOverrideCursor(Qt::SizeAllCursor);
        m_dragCursorActive = true;
    }
}

void Tool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsSvgItem::mouseReleaseEvent(event);

    if (event && event->button() == Qt::LeftButton && m_dragCursorActive) {
        QGuiApplication::restoreOverrideCursor();
        m_dragCursorActive = false;
    }
}
