#pragma once

#include <QElapsedTimer>
#include <QFrame>
#include <QFutureWatcher>
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

    QSize sizeHint() const override { return {140, 140}; }

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

private slots:
    void refresh();
    void onDiskSizeDone();

private:
    void slideIn();
    void slideOut();
    bool readCpuTicks(qint64& utime, qint64& stime);
    void scheduleDiskCalc();

    ArcGauge* m_cpuGauge;
    ArcGauge* m_memGauge;
    ArcGauge* m_diskGauge;

    QTimer*             m_timer;
    QPropertyAnimation* m_anim;
    bool                m_open      = false;
    qint64              m_prevUtime = -1;
    qint64              m_prevStime = -1;
    QElapsedTimer       m_elapsed;

    QFutureWatcher<qint64>* m_diskWatcher;
    QTimer*                 m_diskTimer;
};
