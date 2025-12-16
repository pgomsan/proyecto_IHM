#pragma once

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QSvgRenderer>

inline QIcon makeFixedColorSvgIcon(const QString &resourcePath, const QSize &size)
{
    QSvgRenderer renderer(resourcePath);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        renderer.render(&painter);
    }

    QIcon icon;
    icon.addPixmap(pixmap, QIcon::Normal);
    icon.addPixmap(pixmap, QIcon::Active);
    icon.addPixmap(pixmap, QIcon::Selected);
    icon.addPixmap(pixmap, QIcon::Disabled);
    return icon;
}

