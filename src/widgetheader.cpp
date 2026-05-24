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

#include "widgetheader.h"
#include "svgicon.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QToolButton>
#include <QWindow>

// Zinc-palette constants (shadcn/ui neutral scale)
static const QColor ZINC_400(161, 161, 170); // icon color
static const QColor ZINC_700(63, 63, 70);    // hover bg (hex #3f3f46)

static QToolButton *makeHeaderBtn(const QString &iconPath,
                                  const QString &tip,
                                  QWidget *parent)
{
    auto *btn = new QToolButton(parent);
    btn->setIcon(svgIcon(iconPath, ZINC_400, 15));
    btn->setFixedSize(28, 28);
    btn->setAutoRaise(true);
    btn->setToolTip(tip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QStringLiteral(
        "QToolButton  { background:transparent; border:none; border-radius:6px; }"
        "QToolButton:hover { background:#3f3f46; }"));
    return btn;
}

WidgetHeader::WidgetHeader(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(40);
    setCursor(Qt::SizeAllCursor);
    setAttribute(Qt::WA_StyledBackground);
    setObjectName(QStringLiteral("WidgetHeader"));

    setStyleSheet(QStringLiteral(
        "#WidgetHeader {"
        "  background: #18181b;"
        "  border-top-left-radius:  22px;" // edit these for more rounding
        "  border-top-right-radius: 22px;" // ...
        "  border-bottom: 1px solid #27272a;"
        "}"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 8, 0);
    layout->setSpacing(4);

    auto *logo = new QLabel(QStringLiteral("\U0001D54F"), this); // 𝕏
    logo->setStyleSheet(QStringLiteral(
        "color:#e4e4e7; font-size:14px; font-weight:600; background:transparent;"));
    logo->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *title = new QLabel(QStringLiteral("XCOM"), this);
    QFont f = title->font();
    f.setPixelSize(10);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    title->setFont(f);
    title->setStyleSheet(QStringLiteral("color:#52525b; background:transparent;"));
    title->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *reloadBtn = makeHeaderBtn(
        QStringLiteral(":/icons/rotate-cw.svg"),
        QStringLiteral("Reload"), this);
    auto *exitBtn = makeHeaderBtn(
        QStringLiteral(":/icons/arrow-right.svg"),
        QStringLiteral("Exit Widget Mode"), this);
    auto *hideBtn = makeHeaderBtn(
        QStringLiteral(":/icons/circle-x.svg"),
        QStringLiteral("Hide to tray"), this);

    connect(reloadBtn, &QToolButton::clicked, this, &WidgetHeader::reloadRequested);
    connect(exitBtn, &QToolButton::clicked, this, &WidgetHeader::exitRequested);
    connect(hideBtn, &QToolButton::clicked, this, &WidgetHeader::hideRequested);

    layout->addWidget(logo);
    layout->addSpacing(6);
    layout->addWidget(title);
    layout->addStretch();
    layout->addWidget(reloadBtn);
    layout->addWidget(exitBtn);
    layout->addWidget(hideBtn);
}

void WidgetHeader::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {

        if (QWindow *w = window()->windowHandle())
            w->startSystemMove();
        e->accept();
        return;
    }
    QWidget::mousePressEvent(e);
}
