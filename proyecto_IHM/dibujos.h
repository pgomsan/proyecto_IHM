#ifndef DIBUJOS_H
#define DIBUJOS_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QPointF>
#include <QVector>

class QObject;
class QEvent;

class Dibujos
{
public:
    Dibujos(QGraphicsScene *scene, QGraphicsView *view);

    void setDrawLineMode(bool enabled);
    void setDrawPointMode(bool enabled);
    bool drawLineMode() const { return m_drawLineMode; }
    bool drawPointMode() const { return m_drawPointMode; }

    bool handleEvent(QObject *obj, QEvent *event);
    void reset();

    const QVector<QPointF> &pointCoordinates() const { return m_pointCoordinates; }

private:
    void refreshInteractionMode();
    void clearCurrentLine();
    void addPointAt(const QPointF &scenePos);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;

    bool m_drawLineMode = false;
    bool m_drawPointMode = false;

    QGraphicsLineItem *m_currentLineItem = nullptr;
    QPointF m_lineStart;
    QVector<QGraphicsEllipseItem*> m_pointItems;
    QVector<QPointF> m_pointCoordinates;
};

#endif // DIBUJOS_H
