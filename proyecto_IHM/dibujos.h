#ifndef DIBUJOS_H
#define DIBUJOS_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
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
    bool drawLineMode() const { return m_drawLineMode; }
    bool drawPointMode() const { return m_drawPointMode; }
    void setLineColor(const QColor &color);
    std::pair<double,double> screenToGeo(double pos_x, double pos_y);
    void setPointColor(const QColor &color);
    QColor lineColor() const { return m_lineColor; }
    QColor pointColor() const { return m_pointColor; }

    bool handleEvent(QObject *obj, QEvent *event);
    void reset();

    const QVector<QPointF> &pointCoordinates() const { return m_pointCoordinates; }
    bool erasePointItem(QGraphicsItem *item);

private:
    void refreshInteractionMode();
    void clearCurrentLine();
    void addPointAt(const QPointF &scenePos);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;

    bool m_drawLineMode = false;
    bool m_drawPointMode = false;

    QGraphicsLineItem *m_currentLineItem = nullptr;
    QColor m_lineColor = Qt::red;
    QColor m_pointColor = Qt::red;
    QPointF m_lineStart;
    QVector<QGraphicsEllipseItem*> m_pointItems;
    QVector<QPointF> m_pointCoordinates;
};

#endif // DIBUJOS_H
