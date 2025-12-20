#ifndef COMPASS_TOOL_H
#define COMPASS_TOOL_H

#include <QGraphicsObject>
#include <QPainterPath>
#include <QPoint>
#include <QPointer>
#include <QRectF>
#include <QSizeF>
#include <QString>

class QGraphicsScene;
class QGraphicsSvgItem;
class QGraphicsView;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QEvent;

class CompassTool : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit CompassTool(const QString &legSvgResourcePath,
                         QGraphicsItem *parent = nullptr);

    void setToolSize(const QSizeF &sizePx);
    void setView(QGraphicsView *view);
    bool adjustOpeningSteps(double steps);

    static CompassTool* toggleTool(CompassTool *&tool,
                                   QGraphicsScene *scene,
                                   QGraphicsView *view,
                                   const QString &legResourcePath,
                                   const QSizeF &defaultSize,
                                   const QPoint &initialViewportPos,
                                   bool visible);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    void applyInitialScale();
    void applyLegTransforms();
    void updateGeometry();

    QSizeF m_targetSizePx;
    double m_uniformScale = 1.0;

    double m_fixedAngleDeg = 0.0;
    double m_openingDeg = 35.0;
    double m_angleDeg = 0.0;

    bool m_dragCursorActive = false;

    QGraphicsSvgItem *m_fixedLeg = nullptr;
    QGraphicsSvgItem *m_movingLeg = nullptr;
    QPointer<QGraphicsView> m_view;
};

#endif // COMPASS_TOOL_H
