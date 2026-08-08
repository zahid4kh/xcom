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

#include <QElapsedTimer>
#include <QFrame>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>

class ArcGauge : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double gaugeValue READ gaugeValue WRITE setGaugeValue)
public:
    explicit ArcGauge(const QString& label, const QColor& color,
                      QWidget* parent = nullptr);

    double gaugeValue() const  { return m_value; }
    void   setGaugeValue(double v);
    void   setDisplayText(const QString& t);

    QSize sizeHint() const override { return {128, 128}; }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    double  m_value = 0.0;
    QString m_label;
    QString m_text = QStringLiteral("—");
    QColor  m_color;
};

class ResourcePanel : public QFrame
{
    Q_OBJECT
public:
    explicit ResourcePanel(QWidget* parent = nullptr);

    void toggle();
    void reanchor();

signals:
    void cpuStatsChanged(const QString& text);
    void memStatsChanged(const QString& text);
    void diskStatsChanged(const QString& text);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void refresh();
    void onDiskSizeDone();

private:
    void slideIn();
    void slideOut();
    bool readCpuTicks(qint64& utime, qint64& stime);
    void scheduleDiskCalc();
    QRect centeredRect(const QSize& size) const;

    QSize m_naturalSize;

    ArcGauge* m_cpuGauge;
    ArcGauge* m_memGauge;
    ArcGauge* m_diskGauge;

    QWidget*             m_backdrop      = nullptr;
    QGraphicsOpacityEffect* m_backdropFx = nullptr;
    QPropertyAnimation*  m_backdropAnim  = nullptr;

    QTimer*             m_timer;
    QPropertyAnimation* m_anim;
    bool                m_open      = false;
    qint64              m_prevUtime = -1;
    qint64              m_prevStime = -1;
    QElapsedTimer       m_elapsed;

    QFutureWatcher<qint64>* m_diskWatcher;
    QTimer*                 m_diskTimer;
};
