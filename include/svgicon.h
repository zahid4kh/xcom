#pragma once

#include <QByteArray>
#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPixmap>

inline QIcon svgIcon(const QString& path, const QColor& color, int size = 20)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QByteArray data = f.readAll();

    const QByteArray hex = color.name(QColor::HexRgb).toUtf8();
    data.replace(QByteArrayLiteral("stroke=\"#000000\""),
                 QByteArrayLiteral("stroke=\"") + hex + '"');
    if (!data.contains(QByteArrayLiteral("fill=\"none\"")))
        data.replace(QByteArrayLiteral("<svg "),
                     QByteArrayLiteral("<svg fill=\"") + hex + QByteArrayLiteral("\" "));

    QPixmap pix;
    pix.loadFromData(data, "SVG");
    if (pix.isNull()) return {};
    if (pix.width() != size)
        pix = pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return QIcon(pix);
}
