#ifndef PROGRESSRING_H
#define PROGRESSRING_H
#include <QWidget>
#include <QTimer>
#include <QPainter>

class ProgressRing : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressRing(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(80, 80);
        setAttribute(Qt::WA_TranslucentBackground);

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [&]() {
            angle = (angle + 6) % 360;
            update();
        });
    }

    void startIndeterminate() {
        isIndeterminate = true;
        progress = 0;
        timer->start(16);
        show();
    }

    void startDeterminate() {
        isIndeterminate = false;
        timer->stop();
        show();
    }

    void stop() {
        timer->stop();
        hide();
    }

    void setProgress(double p) {
        progress = qBound(0.0, p, 100.0);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRectF rect(6, 6, width() - 12, height() - 12);

        QPen bgPen(QColor(220,220,220));
        bgPen.setWidth(6);
        p.setPen(bgPen);
        p.drawArc(rect, 0, 360 * 16);

        QPen fgPen(QColor("#2979ff"));
        fgPen.setWidth(6);
        fgPen.setCapStyle(Qt::RoundCap);
        p.setPen(fgPen);

        if (isIndeterminate) {
            p.drawArc(rect, angle * 16, 120 * 16);
        } else {
            int span = int((progress / 100.0) * 360.0 * 16);
            p.drawArc(rect, 90 * 16, -span);
        }

        if (!isIndeterminate) {
            QFont f = font();
            f.setBold(true);
            f.setPointSize(12);
            p.setFont(f);
            p.setPen(Qt::black);
            p.drawText(rect, Qt::AlignCenter,
                       QString::number(int(progress)) + "%");
        }
    }

private:
    QTimer *timer;
    int angle = 0;
    double progress = 0;
    bool isIndeterminate = true;
};


#endif // PROGRESSRING_H
