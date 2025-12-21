#pragma once
#include <QWidget>
#include <QTimer>

class TrackIndicator : public QWidget {
    Q_OBJECT
public:
    explicit TrackIndicator(QWidget* parent = nullptr);
    void start();
    void stop();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QTimer* timer;
};
