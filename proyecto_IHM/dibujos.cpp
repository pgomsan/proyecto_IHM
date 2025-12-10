#include "dibujos.h"
#include <utility>
#include <cmath>

#include <QEvent>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QLineF>
#include <QObject>

double tamaño_punto = 15.0;


DMS Dibujos::decimalToDMS(double value, bool isLatitude)
{
    DMS dms;



    // Valor absoluto para el cálculo
    double absVal = std::fabs(value);

    // Grados
    dms.degrees = static_cast<int>(absVal);

    // Minutos
    double minutesFull = (absVal - dms.degrees) * 60.0;
    dms.minutes = static_cast<int>(minutesFull);

    // Segundos
    dms.seconds = (minutesFull - dms.minutes) * 60.0;

    return dms;
}
QString Dibujos::formatDMS(double value, bool isLatitude)
{
    DMS d = decimalToDMS(value, isLatitude);

    // anchura: lat 2 dígitos, lon 3
    int degWidth = isLatitude ? 2 : 3;

    return QString("%1° %2' %3\"")
        .arg(d.degrees, degWidth, 10, QLatin1Char('0'))   // grados con ceros delante
        .arg(d.minutes, 2, 10, QLatin1Char('0'))          // minutos 2 dígitos
        .arg(d.seconds, 0, 'f', 2);                       // segundos con 2 decimales
}

Dibujos::Dibujos(QGraphicsScene *scene, QGraphicsView *view)
    : m_scene(scene)
    , m_view(view)
{
    refreshInteractionMode();
}

void Dibujos::setDrawLineMode(bool enabled)
{
    if (enabled && m_drawPointMode) {
        m_drawPointMode = false;
    }

    m_drawLineMode = enabled;

    if (!m_drawLineMode) {
        clearCurrentLine();
    }

    refreshInteractionMode();
}

void Dibujos::setDrawPointMode(bool enabled)
{
    if (enabled && m_drawLineMode) {
        m_drawLineMode = false;
        clearCurrentLine();
    }

    m_drawPointMode = enabled;
    refreshInteractionMode();
}

void Dibujos::setLineColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }

    m_lineColor = color;

    if (m_currentLineItem) {
        QPen pen = m_currentLineItem->pen();
        pen.setColor(m_lineColor);
        m_currentLineItem->setPen(pen);
    }
}

void Dibujos::setPointColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }

    m_pointColor = color;
}

bool Dibujos::handleEvent(QObject *obj, QEvent *event)
{
    if (!m_view || !m_scene || obj != m_view->viewport()) {
        return false;
    }

    if (m_drawLineMode) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton) {
                m_lineStart = m_view->mapToScene(e->pos());

                QPen pen(m_lineColor, 8);
                m_currentLineItem = new QGraphicsLineItem();
                m_currentLineItem->setZValue(10);
                m_currentLineItem->setPen(pen);
                m_currentLineItem->setLine(QLineF(m_lineStart, m_lineStart));
                m_scene->addItem(m_currentLineItem);

                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *e = static_cast<QMouseEvent*>(event);
            if ((e->buttons() & Qt::RightButton) && m_currentLineItem) {
                QPointF p2 = m_view->mapToScene(e->pos());
                m_currentLineItem->setLine(QLineF(m_lineStart, p2));
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton && m_currentLineItem) {
                QLineF line = m_currentLineItem->line();
                if (line.length() < 2.0) {
                    m_scene->removeItem(m_currentLineItem);
                    delete m_currentLineItem;
                } else {
                    m_lineItems.append(m_currentLineItem);
                }
                m_currentLineItem = nullptr;
                return true;
            }
        }
    } else if (m_drawPointMode) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton) {
                addPointAt(m_view->mapToScene(e->pos()));
                return true;
            }
        }
    }

    return false;
}

void Dibujos::reset()
{
    m_drawLineMode = false;
    m_drawPointMode = false;
    clearCurrentLine();

    for (QGraphicsLineItem *line : m_lineItems) {
        m_scene->removeItem(line);
        delete line;
    }
    m_lineItems.clear();

    for (QGraphicsEllipseItem *point : m_pointItems) {
        m_scene->removeItem(point);
        delete point;
    }
    m_pointItems.clear();
    m_pointCoordinates.clear();

    refreshInteractionMode();
}

void Dibujos::refreshInteractionMode()
{
    if (!m_view) {
        return;
    }
    if (m_drawLineMode || m_drawPointMode) {
        m_view->setDragMode(QGraphicsView::NoDrag);
        m_view->setCursor(Qt::CrossCursor);
    } else {
        m_view->setDragMode(QGraphicsView::ScrollHandDrag);
        m_view->unsetCursor();
    }
}

void Dibujos::clearCurrentLine()
{
    if (m_currentLineItem) {
        m_scene->removeItem(m_currentLineItem);
        delete m_currentLineItem;
        m_currentLineItem = nullptr;
    }
}

std::pair<double,double> Dibujos::screenToGeo(double pos_x, double pos_y)
{
    //Coordenadas de la imagen del mapa en pantalla
    const double x_min = 321.0;
    const double x_max = 8369.0;
    const double y_min = 437.0;
    const double y_max = 5332.8;

    //Coordenadas geográficas reales del mapa

    const double lat_top    = 36.20;
    const double lat_bottom = 35.60;
    const double lon_left   = -6.40;
    const double lon_right  = -4.90;

    // Tamaño total
    double map_width  = x_max - x_min;
    double map_height = y_max - y_min;

    // Normalización a coordenadas [0,1]
    double u = (pos_x - x_min) / map_width;
    double v = (pos_y - y_min) / map_height;

    // Interpolación geográfica
    double lon = lon_left + u * (lon_right - lon_left);
    double lat = lat_top - v * (lat_top - lat_bottom);

    return {lat, lon};
}

void Dibujos::addPointAt(const QPointF &scenePos)
{
    static const qreal kRadius = tamaño_punto;
    QPen pen(m_pointColor, 2.0);
    QBrush brush(m_pointColor);

    auto [lat, lon] = screenToGeo(scenePos.x(), scenePos.y());

    QString latStr = formatDMS(lat,  true);  // true  = es latitud
    QString lonStr = formatDMS(lon, false);  // false = es longitud

    auto *pointItem = m_scene->addEllipse(scenePos.x() - kRadius,
                                          scenePos.y() - kRadius,
                                          kRadius * 2.0,
                                          kRadius * 2.0,
                                          pen,
                                          brush);
    pointItem->setZValue(12);
    pointItem->setToolTip(
        QObject::tr("Punto %1\nLatitud = %2\nLongitud = %3")
            .arg(m_pointCoordinates.size() + 1)
            .arg(latStr)
            .arg(lonStr));

    m_pointItems.append(pointItem);
    m_pointCoordinates.append(scenePos);
}


bool Dibujos::erasePointItem(QGraphicsItem *item)
{
    auto *point = qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
    if (!point) {
        return false;
    }

    const int idx = m_pointItems.indexOf(point);
    if (idx == -1) {
        return false;
    }

    m_scene->removeItem(point);
    delete point;
    m_pointItems.removeAt(idx);
    m_pointCoordinates.removeAt(idx);
    return true;
}

bool Dibujos::eraseLineItem(QGraphicsItem *item)
{
    auto *line = qgraphicsitem_cast<QGraphicsLineItem*>(item);
    if (!line) {
        return false;
    }

    const int idx = m_lineItems.indexOf(line);
    if (idx == -1) {
        return false;
    }

    m_scene->removeItem(line);
    delete line;
    m_lineItems.removeAt(idx);
    return true;
}
