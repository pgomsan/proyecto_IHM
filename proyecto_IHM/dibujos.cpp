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
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QtMath>

double tamanyo_punto = 15.0;


DMS Dibujos::decimalToDMS(double value, bool isLatitude)
{
    DMS dms;



    // Valor absoluto para el cÃ¡lculo
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

    // anchura: lat 2 dÃ­gitos, lon 3
    int degWidth = isLatitude ? 2 : 3;

    return QString("%1Â° %2' %3\"")
        .arg(d.degrees, degWidth, 10, QLatin1Char('0'))   // grados con ceros delante
        .arg(d.minutes, 2, 10, QLatin1Char('0'))          // minutos 2 dÃ­gitos
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
    if (enabled) {
        if (m_drawPointMode) {
            m_drawPointMode = false;
        }
        if (m_drawArcMode) {
            m_drawArcMode = false;
            clearCurrentArc();
        }
    }

    m_drawLineMode = enabled;

    if (!m_drawLineMode) {
        clearCurrentLine();
    }

    refreshInteractionMode();
}

void Dibujos::setDrawPointMode(bool enabled)
{
    if (enabled) {
        if (m_drawLineMode) {
            m_drawLineMode = false;
            clearCurrentLine();
        }
        if (m_drawArcMode) {
            m_drawArcMode = false;
            clearCurrentArc();
        }
    }

    m_drawPointMode = enabled;
    refreshInteractionMode();
}

void Dibujos::setDrawArcMode(bool enabled)
{
    if (enabled) {
        if (m_drawLineMode) {
            m_drawLineMode = false;
            clearCurrentLine();
        }
        if (m_drawPointMode) {
            m_drawPointMode = false;
        }
    } else {
        clearCurrentArc();
    }

    m_drawArcMode = enabled;
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
    if (m_currentArcItem) {
        QPen pen = m_currentArcItem->pen();
        pen.setColor(m_lineColor);
        m_currentArcItem->setPen(pen);
    }
    if (m_radiusGuide) {
        QPen pen = m_radiusGuide->pen();
        pen.setColor(m_lineColor);
        m_radiusGuide->setPen(pen);
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

    if (m_drawArcMode) {
        // Fase 1: click derecho fija centro; arrastre y suelta para fijar radio/punto inicial;
        // Fase 2: nuevo click derecho + arrastre barre el arco; suelta para fijar.
        if (event->type() == QEvent::MouseButtonPress) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton) {
                if (!m_arcStartLocked) {
                    // Inicio: fijamos centro
                    clearCurrentArc();
                    m_arcCenter = m_view->mapToScene(e->pos());

                    QPen pen(m_lineColor, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    m_currentArcItem = new QGraphicsPathItem();
                    m_currentArcItem->setZValue(11);
                    m_currentArcItem->setPen(pen);
                    m_scene->addItem(m_currentArcItem);

                    QPen guidePen(m_lineColor, 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
                    m_radiusGuide = new QGraphicsLineItem();
                    m_radiusGuide->setZValue(11);
                    m_radiusGuide->setPen(guidePen);
                    m_scene->addItem(m_radiusGuide);

                    m_arcDraggingRadius = true;
                    m_arcSweeping = false;
                    return true;
                } else {
                    // Segunda fase: comenzar a barrer el arco
                    m_arcSweeping = true;
                    m_arcDraggingRadius = false;
                    m_arcLastDeg = m_arcStartDeg;
                    // Si hay guia, mantenla como indicador del radio fijo
                    if (m_radiusGuide) {
                        const double rad = qDegreesToRadians(m_arcStartDeg);
                        const QPointF startPoint(m_arcCenter.x() + std::cos(rad) * m_arcRadius,
                                                 m_arcCenter.y() - std::sin(rad) * m_arcRadius);
                        m_radiusGuide->setLine(QLineF(m_arcCenter, startPoint));
                    }
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *e = static_cast<QMouseEvent*>(event);
            if ((e->buttons() & Qt::RightButton) && m_currentArcItem) {
                const QPointF scenePos = m_view->mapToScene(e->pos());
                if (m_arcDraggingRadius) {
                    QLineF line(m_arcCenter, scenePos);
                    const double r = line.length();
                    if (m_radiusGuide) {
                        m_radiusGuide->setLine(line);
                    }
                } else if (m_arcSweeping && m_arcStartLocked) {
                    updateArcPreview(scenePos);
                }
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::RightButton && m_currentArcItem) {
                if (m_arcDraggingRadius) {
                    // Soltar define radio y punto inicial
                    QLineF line(m_arcCenter, m_view->mapToScene(e->pos()));
                    const double r = line.length();
                    if (m_radiusGuide) {
                        m_radiusGuide->setLine(line);
                    }
                    if (r >= 4.0) {
                        m_arcRadius = r;
                        m_arcStartDeg = qRadiansToDegrees(std::atan2(-line.dy(), line.dx()));
                        m_arcSpanDeg = 0.0;
                        m_arcLastDeg = m_arcStartDeg;
                        m_arcStartLocked = true;
                        m_arcDraggingRadius = false;
                        m_arcSweeping = false;
                    } else {
                        clearCurrentArc();
                    }
                    return true;
                }

                if (m_arcSweeping && m_arcStartLocked) {
                    const bool valid = std::fabs(m_arcSpanDeg) > 1.0;
                    if (!valid) {
                        clearCurrentArc();
                    } else {
                        m_arcItems.append(m_currentArcItem);
                        m_currentArcItem = nullptr;
                        if (m_radiusGuide) {
                            m_scene->removeItem(m_radiusGuide);
                            delete m_radiusGuide;
                            m_radiusGuide = nullptr;
                        }
                        m_arcStartLocked = false;
                        m_arcRadius = 0.0;
                        m_arcStartDeg = 0.0;
                        m_arcSpanDeg = 0.0;
                        m_arcLastDeg = 0.0;
                        m_arcSweeping = false;
                    }
                    return true;
                }
            }
        }
    } else if (m_drawLineMode) {
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
    m_drawArcMode = false;
    clearCurrentLine();
    clearCurrentArc();

    for (QGraphicsLineItem *line : m_lineItems) {
        m_scene->removeItem(line);
        delete line;
    }
    m_lineItems.clear();

    for (QGraphicsPathItem *arc : m_arcItems) {
        m_scene->removeItem(arc);
        delete arc;
    }
    m_arcItems.clear();

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
    if (m_drawLineMode || m_drawPointMode || m_drawArcMode) {
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

void Dibujos::clearCurrentArc()
{
    if (m_currentArcItem) {
        m_scene->removeItem(m_currentArcItem);
        delete m_currentArcItem;
        m_currentArcItem = nullptr;
    }
    if (m_radiusGuide) {
        m_scene->removeItem(m_radiusGuide);
        delete m_radiusGuide;
        m_radiusGuide = nullptr;
    }
    m_arcRadius = 0.0;
    m_arcStartDeg = 0.0;
    m_arcSpanDeg = 0.0;
    m_arcLastDeg = 0.0;
    m_arcStartLocked = false;
    m_arcDraggingRadius = false;
    m_arcSweeping = false;
}

std::pair<double,double> Dibujos::screenToGeo(double pos_x, double pos_y)
{
    //Coordenadas de la imagen del mapa en pantalla
    const double x_min = 321.0;
    const double x_max = 8369.0;
    const double y_min = 437.0;
    const double y_max = 5332.8;

    //Coordenadas geogrÃ¡ficas reales del mapa

    const double lat_top    = 36.20;
    const double lat_bottom = 35.60;
    const double lon_left   = -6.40;
    const double lon_right  = -4.90;

    // Tamaaño total
    double map_width  = x_max - x_min;
    double map_height = y_max - y_min;

    // Normalizacion a coordenadas [0,1]
    double u = (pos_x - x_min) / map_width;
    double v = (pos_y - y_min) / map_height;

    // Interpolacion geografica
    double lon = lon_left + u * (lon_right - lon_left);
    double lat = lat_top - v * (lat_top - lat_bottom);

    return {lat, lon};
}

void Dibujos::updateArcPreview(const QPointF &scenePos)
{
    if (!m_currentArcItem || m_arcRadius <= 0.0) {
        return;
    }

    const QPointF v = scenePos - m_arcCenter;
    const double currentDeg = qRadiansToDegrees(std::atan2(-v.y(), v.x()));

    // Acumular delta normalizado para poder superar 180/360 grados sin saltos
    auto normalizeDelta = [](double delta) {
        delta = std::fmod(delta + 540.0, 360.0) - 180.0; // en (-180,180]
        return delta;
    };
    const double delta = normalizeDelta(currentDeg - m_arcLastDeg);
    m_arcSpanDeg += delta;
    m_arcLastDeg = currentDeg;

    QRectF box(m_arcCenter.x() - m_arcRadius,
               m_arcCenter.y() - m_arcRadius,
               m_arcRadius * 2.0,
               m_arcRadius * 2.0);

    QPainterPath path;
    path.arcMoveTo(box, m_arcStartDeg);
    path.arcTo(box, m_arcStartDeg, m_arcSpanDeg);
    m_currentArcItem->setPath(path);

    if (m_radiusGuide) {
        const double endDeg = m_arcStartDeg + m_arcSpanDeg;
        const double rad = qDegreesToRadians(endDeg);
        const QPointF endPoint(m_arcCenter.x() + std::cos(rad) * m_arcRadius,
                               m_arcCenter.y() - std::sin(rad) * m_arcRadius);
        m_radiusGuide->setLine(QLineF(m_arcCenter, endPoint));
    }
}

void Dibujos::addPointAt(const QPointF &scenePos)
{
    static const qreal kRadius = tamanyo_punto;
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

bool Dibujos::eraseArcItem(QGraphicsItem *item, const QPointF &scenePos)
{
    auto *arc = qgraphicsitem_cast<QGraphicsPathItem*>(item);
    if (!arc) {
        return false;
    }

    const int idx = m_arcItems.indexOf(arc);
    if (idx == -1) {
        return false;
    }

    // Reducir el area de borrado para evitar borrar "a distancia": comprobamos distancia a la trayectoria
    const QPointF localPos = arc->mapFromScene(scenePos);
    QPainterPathStroker stroker;
    stroker.setWidth(10.0); // tolerancia de borrado mas ajustada que la caja completa del arco
    QPainterPath hitArea = stroker.createStroke(arc->path());
    if (!hitArea.contains(localPos)) {
        return false;
    }

    m_scene->removeItem(arc);
    delete arc;
    m_arcItems.removeAt(idx);
    return true;
}
