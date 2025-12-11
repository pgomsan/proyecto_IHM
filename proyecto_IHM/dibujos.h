#ifndef DIBUJOS_H
#define DIBUJOS_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QColor>
#include <QPointF>
#include <utility>

#include <QChar>
#include <QString>
#include <QVector>

class QObject;
class QEvent;
class QGraphicsItem;

struct DMS {
    int degrees;
    int minutes;
    double seconds;
};

class Dibujos
{
public:
    Dibujos(QGraphicsScene *scene, QGraphicsView *view);
    DMS decimalToDMS(double value, bool isLatitude);
    QString formatDMS(double value, bool isLatitude);
    void setDrawLineMode(bool enabled);
    void setDrawPointMode(bool enabled);
    void setDrawArcMode(bool enabled);
    bool drawLineMode() const { return m_drawLineMode; }
    bool drawPointMode() const { return m_drawPointMode; }
    bool drawArcMode() const { return m_drawArcMode; }
    void setLineColor(const QColor &color);
    std::pair<double,double> screenToGeo(double pos_x, double pos_y);
    void setPointColor(const QColor &color);
    QColor lineColor() const { return m_lineColor; }
    QColor pointColor() const { return m_pointColor; }

    bool handleEvent(QObject *obj, QEvent *event);
    void reset();

    const QVector<QPointF> &pointCoordinates() const { return m_pointCoordinates; }
    bool erasePointItem(QGraphicsItem *item);
    bool eraseLineItem(QGraphicsItem *item);
    bool eraseArcItem(QGraphicsItem *item, const QPointF &scenePos);

private:
    void refreshInteractionMode();
    void clearCurrentLine();
    void addPointAt(const QPointF &scenePos);
    void clearCurrentArc();
    void updateArcPreview(const QPointF &scenePos);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;

    bool m_drawLineMode = false;
    bool m_drawPointMode = false;
    bool m_drawArcMode = false;

    QGraphicsLineItem *m_currentLineItem = nullptr;
    QGraphicsPathItem *m_currentArcItem = nullptr;
    QGraphicsLineItem *m_radiusGuide = nullptr;
    QColor m_lineColor = Qt::red;
    QColor m_pointColor = Qt::red;
    QPointF m_lineStart;
    QPointF m_arcCenter;
    double m_arcRadius = 0.0;
    double m_arcStartDeg = 0.0;
    double m_arcSpanDeg = 0.0;
    double m_arcLastDeg = 0.0;
    bool m_arcStartLocked = false;
    bool m_arcDraggingRadius = false;
    bool m_arcSweeping = false;
    QVector<QGraphicsEllipseItem*> m_pointItems;
    QVector<QGraphicsLineItem*> m_lineItems;
    QVector<QGraphicsPathItem*> m_arcItems;
    QVector<QPointF> m_pointCoordinates;
};

#endif // DIBUJOS_H
