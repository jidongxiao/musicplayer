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
    // Playback state
    bool isPlaying = false;

    // Audio
    QMediaPlayer player;
    QAudioOutput audio;

    // UI elements
    QTableWidget* playlistView;

    QPushButton* openBtn;
    QPushButton* removeBtn;

    QPushButton* shuffleBtn;
    QPushButton* prevBtn;
    QPushButton* playPauseBtn;
    QPushButton* nextBtn;
    QPushButton* repeatBtn;

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
