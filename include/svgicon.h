// Copyright 2026 Zahid Khalilov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
