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
#include <QEvent>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QRegion>
#include <QResizeEvent>
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

    const int pw = 8;
    const int mg = pw + 11;
    const QRectF arc = QRectF(rect()).adjusted(mg, mg, -mg, -mg - 15);

    p.setPen(QPen(QColor(22, 27, 36), pw, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arc, 225 * 16, -270 * 16);

    if (m_value > 0.001) {
        QColor glow = m_color;
        glow.setAlpha(55);
        p.setPen(QPen(glow, pw + 10, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arc, 225 * 16, -qRound(m_value * 270 * 16));

        p.setPen(QPen(m_color, pw, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arc, 225 * 16, -qRound(m_value * 270 * 16));
    }

    QFont vf = font();
    vf.setPixelSize(15);
    vf.setBold(true);
    p.setFont(vf);
    p.setPen(QColor(224, 250, 255));
    p.drawText(arc.toRect(), Qt::AlignCenter, m_text);

    QFont lf = font();
    lf.setPixelSize(10);
    lf.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    p.setFont(lf);
    p.setPen(QColor(70, 140, 155));
    p.drawText(QRect(0, rect().height() - 15, rect().width(), 15),
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
    setAttribute(Qt::WA_StyledBackground);
    setFixedWidth(300);
    setStyleSheet(QStringLiteral(
        "ResourcePanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #05050d, stop:1 #0a0e1a);"
        "  border: 1.5px solid rgba(0,229,255,140);"
        "  border-radius: 18px;"
        "}"
    ));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 16);
    root->setSpacing(6);

    auto* header = new QHBoxLayout;

    auto* dot = new QLabel(QStringLiteral("⊙"), this);
    dot->setStyleSheet(QStringLiteral("color:#00e5ff; font-size:16px;"));

    auto* title = new QLabel(QStringLiteral("Resource Monitor"), this);
    QFont tf = title->font();
    tf.setPixelSize(12);
    tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet(QStringLiteral("color:#c7f8ff; letter-spacing:1px;"));

    auto* closeBtn = new QPushButton(QStringLiteral("✕"), this);
    closeBtn->setFixedSize(22, 22);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; color:#3d5866; border:none; font-size:13px; }"
        "QPushButton:hover { color:#ff4d6d; }"
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
    sep->setStyleSheet(QStringLiteral("color:rgba(0,229,255,40);"));
    root->addWidget(sep);
    root->addSpacing(4);

    m_cpuGauge  = new ArcGauge(QStringLiteral("CPU"),   QColor(0,   229, 255), this);
    m_memGauge  = new ArcGauge(QStringLiteral("MEMORY"), QColor(255,  43, 214), this);
    m_diskGauge = new ArcGauge(QStringLiteral("CACHE"), QColor(57,  255, 136), this);

    const auto addGaugeCard = [&](ArcGauge* g) {
        auto* card = new QFrame(this);
        card->setStyleSheet(QStringLiteral(
            "QFrame { background:#0c0f18; border-radius:14px;"
            "  border:1px solid rgba(0,229,255,45); }"
        ));
        auto* row = new QHBoxLayout(card);
        row->setContentsMargins(0, 8, 0, 8);
        row->addStretch();
        row->addWidget(g);
        row->addStretch();
        root->addWidget(card);
    };

    addGaugeCard(m_cpuGauge);
    addGaugeCard(m_memGauge);
    addGaugeCard(m_diskGauge);

    auto* note = new QLabel(QStringLiteral("Updates every 2 s"), this);
    note->setAlignment(Qt::AlignCenter);
    note->setStyleSheet(QStringLiteral("color:#33505c; font-size:9px; margin-top:2px;"));
    root->addWidget(note);

    adjustSize();
    m_naturalSize = size();

    if (parent) {
        m_backdrop = new QWidget(parent);
        m_backdrop->setAttribute(Qt::WA_StyledBackground);
        m_backdrop->setStyleSheet(QStringLiteral("background: rgba(0,0,0,140);"));
        m_backdropFx = new QGraphicsOpacityEffect(m_backdrop);
        m_backdropFx->setOpacity(0.0);
        m_backdrop->setGraphicsEffect(m_backdropFx);
        m_backdrop->installEventFilter(this);
        m_backdrop->hide();

        m_backdropAnim = new QPropertyAnimation(m_backdropFx, "opacity", this);
    }

    m_anim = new QPropertyAnimation(this, "geometry", this);
    connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
        if (!m_open) {
            hide();
            if (m_backdrop) m_backdrop->hide();
        }
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

    // Sampling runs continuously (not just while the panel is open) so the
    // collapsed toolbar readout always reflects live values.
    refresh();
    m_timer->start();
    scheduleDiskCalc();
    m_diskTimer->start();

    hide();
}

QRect ResourcePanel::centeredRect(const QSize& size) const
{
    QWidget* p = parentWidget();
    const QPoint c = p ? p->rect().center() : QPoint(0, 0);
    return QRect(c.x() - size.width() / 2, c.y() - size.height() / 2,
                 size.width(), size.height());
}

void ResourcePanel::toggle()
{
    m_open ? slideOut() : slideIn();
}

void ResourcePanel::slideIn()
{
    m_open = true;

    if (m_backdrop) {
        m_backdrop->setGeometry(parentWidget()->rect());
        m_backdrop->show();
        m_backdrop->raise();
    }

    const QRect target = centeredRect(m_naturalSize);
    const QRect start  = centeredRect(m_naturalSize * 0.72);

    setGeometry(start);
    raise();
    show();

    m_anim->stop();
    m_anim->setEasingCurve(QEasingCurve::OutBack);
    m_anim->setDuration(320);
    m_anim->setStartValue(start);
    m_anim->setEndValue(target);
    m_anim->start();

    if (m_backdropAnim) {
        m_backdropAnim->stop();
        m_backdropAnim->setEasingCurve(QEasingCurve::OutCubic);
        m_backdropAnim->setDuration(220);
        m_backdropAnim->setStartValue(0.0);
        m_backdropAnim->setEndValue(1.0);
        m_backdropAnim->start();
    }
}

void ResourcePanel::slideOut()
{
    m_open = false;

    const QRect end = centeredRect(m_naturalSize * 0.72);

    m_anim->stop();
    m_anim->setEasingCurve(QEasingCurve::InCubic);
    m_anim->setDuration(200);
    m_anim->setStartValue(geometry());
    m_anim->setEndValue(end);
    m_anim->start();

    if (m_backdropAnim) {
        m_backdropAnim->stop();
        m_backdropAnim->setEasingCurve(QEasingCurve::InCubic);
        m_backdropAnim->setDuration(200);
        m_backdropAnim->setStartValue(1.0);
        m_backdropAnim->setEndValue(0.0);
        m_backdropAnim->start();
    }
}

void ResourcePanel::reanchor()
{
    QWidget* p = parentWidget();
    if (!p) return;

    if (m_backdrop && (m_backdrop->isVisible()))
        m_backdrop->setGeometry(p->rect());

    if (m_open && m_anim->state() != QAbstractAnimation::Running)
        setGeometry(centeredRect(m_naturalSize));
}

bool ResourcePanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_backdrop && event->type() == QEvent::MouseButtonPress) {
        slideOut();
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

void ResourcePanel::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    QPainterPath path;
    path.addRoundedRect(rect(), 18, 18);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
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
            emit cpuStatsChanged(QStringLiteral("…"));
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
                const QString text = QStringLiteral("%1%").arg(qRound(pct));
                m_cpuGauge->setDisplayText(text);
                emit cpuStatsChanged(text);
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
                    const QString text =
                        mb < 1024 ? QStringLiteral("%1 MB").arg(qRound(mb))
                                  : QStringLiteral("%1 GB").arg(mb / 1024.0, 0, 'f', 1);
                    m_memGauge->setDisplayText(text);
                    emit memStatsChanged(text);
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
    const QString text =
        mb < 1024 ? QStringLiteral("%1 MB").arg(qRound(mb))
                  : QStringLiteral("%1 GB").arg(mb / 1024.0, 0, 'f', 1);
    m_diskGauge->setDisplayText(text);
    emit diskStatsChanged(text);
}
