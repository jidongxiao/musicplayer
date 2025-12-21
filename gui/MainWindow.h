#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

#include "PlaylistImpl.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    // Core (student logic)
    PlaylistImpl playlist;
    int currentIndex = -1;

    // Audio
    QMediaPlayer player;
    QAudioOutput audio;

    // UI elements
    QTableWidget* playlistView;

    QPushButton* openBtn;
    QPushButton* playBtn;
    QPushButton* pauseBtn;
    QPushButton* nextBtn;
    QPushButton* prevBtn;
    QPushButton* removeBtn;

    QSlider* progressSlider;
    QLabel*  timeLabel;

    // Helpers
    void setupUi();
    void connectSignals();
    void addTrackFromFile();
    void playTrack(const Track& t);
    void removeSelectedTrack();
    void updateTrackNumbers();
};
