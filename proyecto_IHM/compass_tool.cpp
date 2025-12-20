#include "compass_tool.h"

#include <QtMath>
#include <algorithm>
#include <QApplication>
#include <QGuiApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>
#include <QGraphicsView>

namespace
{
class CompassLegItem final : public QGraphicsSvgItem
{
public:
    explicit CompassLegItem(const QString &fileName, QGraphicsItem *parent = nullptr)
        : QGraphicsSvgItem(fileName, parent)
    {
        setAcceptedMouseButtons(Qt::NoButton);
        setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    }

protected:
    // No wheel behavior for compass legs.
};

constexpr QPointF kLegHingeLocal(0.0, 10.0);
constexpr double kHingePickRadiusPx = 12.0;
}

CompassTool::CompassTool(const QString &legSvgResourcePath, QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    setFlags(QGraphicsItem::ItemIsMovable
             | QGraphicsItem::ItemIsSelectable
             | QGraphicsItem::ItemSendsGeometryChanges
             | QGraphicsItem::ItemIgnoresTransformations);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setTransformOriginPoint(QPointF(0.0, 0.0));

    m_fixedLeg = new CompassLegItem(legSvgResourcePath, this);
    m_movingLeg = new CompassLegItem(legSvgResourcePath, this);

    m_fixedLeg->setTransformOriginPoint(kLegHingeLocal);
    m_movingLeg->setTransformOriginPoint(kLegHingeLocal);

    m_fixedLeg->setPos(-kLegHingeLocal);
    m_movingLeg->setPos(-kLegHingeLocal);

    m_targetSizePx = m_fixedLeg->boundingRect().size();
    applyInitialScale();
    applyLegTransforms();
    updateGeometry();
}

void CompassTool::setToolSize(const QSizeF &sizePx)
{
    m_targetSizePx = sizePx;
    applyInitialScale();
    updateGeometry();
}

void CompassTool::setView(QGraphicsView *view)
{
    m_view = view;
}

CompassTool* CompassTool::toggleTool(CompassTool *&tool,
                                     QGraphicsScene *scene,
                                     QGraphicsView *view,
                                     const QString &legResourcePath,
                                     const QSizeF &defaultSize,
                                     const QPoint &initialViewportPos,
                                     bool visible)
{
    if (!scene || !view) {
        return tool;
    }

    if (!tool) {
        tool = new CompassTool(legResourcePath);
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
        tool->setZValue(1000);
        scene->update();
    }

    return tool;
}

QRectF CompassTool::boundingRect() const
{
    QRectF r;

    if (m_fixedLeg) {
        r = r.united(m_fixedLeg->mapToParent(m_fixedLeg->boundingRect()).boundingRect());
    }
    if (m_movingLeg) {
        r = r.united(m_movingLeg->mapToParent(m_movingLeg->boundingRect()).boundingRect());
    }

    r = r.united(QRectF(-kHingePickRadiusPx,
                        -kHingePickRadiusPx,
                        2.0 * kHingePickRadiusPx,
                        2.0 * kHingePickRadiusPx));

    return r.adjusted(-2.0, -2.0, 2.0, 2.0);
}

QPainterPath CompassTool::shape() const
{
    QPainterPath path;
    if (m_fixedLeg) {
        path.addPath(m_fixedLeg->mapToParent(m_fixedLeg->shape()));
    }
    if (m_movingLeg) {
        path.addPath(m_movingLeg->mapToParent(m_movingLeg->shape()));
    }
    path.addEllipse(QPointF(0.0, 0.0), kHingePickRadiusPx, kHingePickRadiusPx);
    return path;
}

void CompassTool::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

void CompassTool::applyInitialScale()
{
    if (!m_fixedLeg) {
        return;
    }

    const QRectF br = m_fixedLeg->boundingRect();
    if (br.isEmpty()) {
        return;
    }

    const double sx = m_targetSizePx.width() / br.width();
    const double sy = m_targetSizePx.height() / br.height();
    m_uniformScale = std::min(sx, sy);
    setScale(m_uniformScale);
}

void CompassTool::applyLegTransforms()
{
    if (!m_fixedLeg || !m_movingLeg) {
        return;
    }

    m_fixedLeg->setRotation(m_fixedAngleDeg);
    m_movingLeg->setRotation(m_fixedAngleDeg + m_openingDeg);
}

void CompassTool::updateGeometry()
{
    prepareGeometryChange();
    if (scene()) {
        scene()->update();
    }
}

void CompassTool::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsObject::mousePressEvent(event);

    if (event && event->button() == Qt::LeftButton && event->isAccepted() && !m_dragCursorActive) {
        QGuiApplication::setOverrideCursor(Qt::SizeAllCursor);
        m_dragCursorActive = true;
    }
}

void CompassTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsObject::mouseReleaseEvent(event);

    if (event && event->button() == Qt::LeftButton && m_dragCursorActive) {
        QGuiApplication::restoreOverrideCursor();
        m_dragCursorActive = false;
    }
}

void CompassTool::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (!event || m_dragCursorActive) {
        QGraphicsObject::hoverMoveEvent(event);
        return;
    }

    setCursor(Qt::SizeAllCursor);

    QGraphicsObject::hoverMoveEvent(event);
}

void CompassTool::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (!m_dragCursorActive) {
        unsetCursor();
    }
    QGraphicsObject::hoverLeaveEvent(event);
}
