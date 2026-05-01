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

#include "resourcepanel.h"

#include <QDirIterator>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <unistd.h>

ArcGauge::ArcGauge(const QString& label, const QColor& color, QWidget* parent)
    : QWidget(parent), m_label(label), m_color(color)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ArcGauge::setGaugeValue(double v)
{
    m_value = qBound(0.0, v, 1.0);
    update();
}

void ArcGauge::setDisplayText(const QString& t)
{
    m_text = t;
    update();
}

void ArcGauge::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int pw = 9;
    const int mg = pw + 12;
    const QRectF arc = QRectF(rect()).adjusted(mg, mg, -mg, -mg - 16);

    p.setPen(QPen(QColor(35, 35, 60), pw, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arc, 225 * 16, -270 * 16);

    if (m_value > 0.001) {
        QColor glow = m_color;
        glow.setAlpha(50);
        p.setPen(QPen(glow, pw + 10, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arc, 225 * 16, -qRound(m_value * 270 * 16));

        p.setPen(QPen(m_color, pw, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arc, 225 * 16, -qRound(m_value * 270 * 16));
    }

    QFont vf = font();
    vf.setPixelSize(15);
    vf.setBold(true);
    p.setFont(vf);
    p.setPen(QColor(235, 235, 255));
    p.drawText(arc.toRect(), Qt::AlignCenter, m_text);

    QFont lf = font();
    lf.setPixelSize(10);
    p.setFont(lf);
    p.setPen(QColor(110, 110, 170));
    p.drawText(QRect(0, rect().height() - 16, rect().width(), 16),
               Qt::AlignCenter, m_label);
}

static qint64 calcDirSize(const QString& path)
{
    qint64 total = 0;
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isFile())
            total += it.fileInfo().size();
    }
    return total;
}

ResourcePanel::ResourcePanel(QWidget* parent)
    : QFrame(parent)
{
    setFixedWidth(280);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(QStringLiteral(
        "ResourcePanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #0b0b1a, stop:1 #0f0f24);"
        "  border-left: 1px solid #252550;"
        "}"
    ));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 18);
    root->setSpacing(6);

    auto* header = new QHBoxLayout;

    auto* dot = new QLabel(QStringLiteral("⊙"), this);
    dot->setStyleSheet(QStringLiteral("color:#00c8ff; font-size:16px;"));

    auto* title = new QLabel(QStringLiteral("Resource Monitor"), this);
    QFont tf = title->font();
    tf.setPixelSize(12);
    tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet(QStringLiteral("color:#c0c0f0; letter-spacing:1px;"));

    auto* closeBtn = new QPushButton(QStringLiteral("✕"), this);
    closeBtn->setFixedSize(22, 22);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; color:#505080; border:none; font-size:13px; }"
        "QPushButton:hover { color:#ffffff; }"
    ));
    connect(closeBtn, &QPushButton::clicked, this, &ResourcePanel::toggle);

    header->addWidget(dot);
    header->addSpacing(6);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(closeBtn);
    root->addLayout(header);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("color:#252550;"));
    root->addWidget(sep);
    root->addSpacing(6);

    m_cpuGauge  = new ArcGauge(QStringLiteral("CPU"),    QColor(0,   200, 255), this);
    m_memGauge  = new ArcGauge(QStringLiteral("Memory"), QColor(160,  90, 255), this);
    m_diskGauge = new ArcGauge(QStringLiteral("Cache"),  QColor(20,  210, 140), this);

    const auto addGaugeCard = [&](ArcGauge* g) {
        auto* card = new QFrame(this);
        card->setStyleSheet(QStringLiteral(
            "QFrame { background:#111128; border-radius:12px;"
            "  border:1px solid #202048; }"
        ));
        auto* row = new QHBoxLayout(card);
        row->setContentsMargins(0, 10, 0, 10);
        row->addStretch();
        row->addWidget(g);
        row->addStretch();
        root->addWidget(card);
    };

    addGaugeCard(m_cpuGauge);
    addGaugeCard(m_memGauge);
    addGaugeCard(m_diskGauge);

    root->addStretch();

    auto* note = new QLabel(QStringLiteral("Updates every 2 s"), this);
    note->setAlignment(Qt::AlignCenter);
    note->setStyleSheet(QStringLiteral("color:#303060; font-size:9px;"));
    root->addWidget(note);

    m_anim = new QPropertyAnimation(this, "pos", this);
    m_anim->setDuration(300);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
        if (!m_open) hide();
    });

    m_timer = new QTimer(this);
    m_timer->setInterval(2000);
    connect(m_timer, &QTimer::timeout, this, &ResourcePanel::refresh);

    m_diskWatcher = new QFutureWatcher<qint64>(this);
    connect(m_diskWatcher, &QFutureWatcher<qint64>::finished,
            this, &ResourcePanel::onDiskSizeDone);

    m_diskTimer = new QTimer(this);
    m_diskTimer->setInterval(30000);
    connect(m_diskTimer, &QTimer::timeout, this, &ResourcePanel::scheduleDiskCalc);

    hide();
    if (parent) move(parent->width(), 0);
}

void ResourcePanel::toggle()
{
    m_open ? slideOut() : slideIn();
}

void ResourcePanel::slideIn()
{
    m_open      = true;
    m_prevUtime = -1;

    QWidget* p = parentWidget();
    setFixedHeight(p->height());
    move(p->width(), 0);
    raise();
    show();

    m_anim->stop();
    m_anim->setStartValue(QPoint(p->width(), 0));
    m_anim->setEndValue(QPoint(p->width() - width(), 0));
    m_anim->start();

    refresh();
    m_timer->start();
    scheduleDiskCalc();
    m_diskTimer->start();
}

void ResourcePanel::slideOut()
{
    m_open = false;
    m_timer->stop();
    m_diskTimer->stop();

    QWidget* p = parentWidget();
    m_anim->stop();
    m_anim->setStartValue(pos());
    m_anim->setEndValue(QPoint(p->width(), 0));
    m_anim->start();
}

void ResourcePanel::reanchor()
{
    QWidget* p = parentWidget();
    if (!p) return;
    setFixedHeight(p->height());
    if (!m_open || m_anim->state() == QAbstractAnimation::Running)
        move(m_open ? p->width() - width() : p->width(), 0);
    else
        move(p->width() - width(), 0);
}

bool ResourcePanel::readCpuTicks(qint64& utime, qint64& stime)
{
    QFile f(QStringLiteral("/proc/self/stat"));
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = f.readAll();
    const int rp = data.lastIndexOf(')');
    if (rp < 0) return false;
    const auto fields = data.mid(rp + 2).split(' ');
    if (fields.size() < 13) return false;
    utime = fields[11].toLongLong();
    stime = fields[12].toLongLong();
    return true;
}

void ResourcePanel::refresh()
{
    qint64 utime = 0, stime = 0;
    if (readCpuTicks(utime, stime)) {
        if (m_prevUtime < 0) {
            m_prevUtime = utime;
            m_prevStime = stime;
            m_elapsed.start();
            m_cpuGauge->setDisplayText(QStringLiteral("…"));
        } else {
            const qint64 elapsedMs  = m_elapsed.elapsed();
            const qint64 deltaTicks = (utime + stime) - (m_prevUtime + m_prevStime);
            m_prevUtime = utime;
            m_prevStime = stime;
            m_elapsed.restart();

            if (elapsedMs > 0) {
                const long clkTck = sysconf(_SC_CLK_TCK);
                double pct = deltaTicks * 100000.0 / (clkTck * elapsedMs);
                pct = qBound(0.0, pct, 100.0);
                m_cpuGauge->setGaugeValue(pct / 100.0);
                m_cpuGauge->setDisplayText(QStringLiteral("%1%").arg(qRound(pct)));
            }
        }
    }

    QFile mf(QStringLiteral("/proc/self/status"));
    if (mf.open(QIODevice::ReadOnly)) {
        for (const QByteArray& line : mf.readAll().split('\n')) {
            if (line.startsWith("VmRSS:")) {
                const auto parts = line.simplified().split(' ');
                if (parts.size() >= 2) {
                    const double mb = parts[1].toLongLong() / 1024.0;
                    m_memGauge->setGaugeValue(qBound(0.0, mb / 2048.0, 1.0));
                    m_memGauge->setDisplayText(
                        mb < 1024 ? QStringLiteral("%1 MB").arg(qRound(mb))
                                  : QStringLiteral("%1 GB").arg(mb / 1024.0, 0, 'f', 1));
                }
                break;
            }
        }
    }
}

void ResourcePanel::scheduleDiskCalc()
{
    if (m_diskWatcher->isRunning()) return;
    m_diskGauge->setDisplayText(QStringLiteral("…"));
    const QString path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_diskWatcher->setFuture(QtConcurrent::run([path]() -> qint64 {
        return calcDirSize(path);
    }));
}

void ResourcePanel::onDiskSizeDone()
{
    const double mb = m_diskWatcher->result() / (1024.0 * 1024.0);
    m_diskGauge->setGaugeValue(qBound(0.0, mb / 1024.0, 1.0));
    m_diskGauge->setDisplayText(
        mb < 1024 ? QStringLiteral("%1 MB").arg(qRound(mb))
                  : QStringLiteral("%1 GB").arg(mb / 1024.0, 0, 'f', 1));
}
