#include "dibujos.h"

#include <QEvent>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QLineF>
#include <QObject>

double tamaño_punto = 15.0;

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

    for (QGraphicsEllipseItem *point : m_pointItems) {
        if (!point) {
            continue;
        }
        point->setPen(QPen(m_pointColor, point->pen().widthF()));
        point->setBrush(QBrush(m_pointColor));
    }
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

void Dibujos::addPointAt(const QPointF &scenePos)
{
    static const qreal kRadius = tamaño_punto;
    QPen pen(m_pointColor, 2.0);
    QBrush brush(m_pointColor);

    auto *pointItem = m_scene->addEllipse(scenePos.x() - kRadius,
                                          scenePos.y() - kRadius,
                                          kRadius * 2.0,
                                          kRadius * 2.0,
                                          pen,
                                          brush);
    pointItem->setZValue(12);
    pointItem->setToolTip(
                QObject::tr("Punto %1\nX: %2\nY: %3")
                .arg(m_pointCoordinates.size() + 1)
                .arg(scenePos.x(), 0, 'f', 1)
                .arg(scenePos.y(), 0, 'f', 1));

    m_pointItems.append(pointItem);
    m_pointCoordinates.append(scenePos);
}
