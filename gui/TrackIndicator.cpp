#include "TrackIndicator.h"
#include <QPainter>
#include <QRandomGenerator>

TrackIndicator::TrackIndicator(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(20, 20);
    timer = new QTimer(this);
    // Use a lambda to explicitly call update():
    connect(timer, &QTimer::timeout, this, [this]() {
        update();
    });
}

void TrackIndicator::start() { timer->start(100); }
void TrackIndicator::stop()  { timer->stop(); }

void TrackIndicator::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int barCount = 5;
    int w = width() / barCount;
    for (int i = 0; i < barCount; ++i) {
        int h = QRandomGenerator::global()->bounded(height());
        p.fillRect(i * w, height() - h, w - 2, h, Qt::green);
    }
}
