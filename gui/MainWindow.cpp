#include "MainWindow.h"
#include "Mp3Reader.h"
#include "TrackIndicator.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QUrl>
#include <QHeaderView>
#include <QEventLoop>
#include <QResource>
#include <QDirIterator>
#include <random>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    audio.setVolume(0.5);
    player.setAudioOutput(&audio);
    // Debug: Walk the entire resource tree
    QDirIterator it(":", QDirIterator::Subdirectories);
    qDebug() << "--- ALL REGISTERED RESOURCES ---";
    while (it.hasNext()) {
        qDebug() << it.next();
    }
    qDebug() << "-------------------------------";
    setupUi();
    connectSignals();
}

void MainWindow::setupUi()
{
    setWindowTitle("Rensselaer Music Player");

    playlistView = new QTableWidget(this);
    playlistView->setColumnCount(5); // #, Title, Artist, Album, Duration
    QStringList headers;
    headers << "#" << "Title" << "Artist" << "Album" << "Duration";
    playlistView->setHorizontalHeaderLabels(headers);

    // Make selection single row
    playlistView->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistView->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QHeaderView* header = playlistView->horizontalHeader();
    playlistView->verticalHeader()->setVisible(false);

    // Column 0: "#" (track number / indicator)
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    playlistView->setColumnWidth(0, 40);

    // Column 1–3: Title / Artist / Album (stretch)
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);

    // Column 4: Duration (fixed)
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    playlistView->setColumnWidth(4, 70);

    openBtn  = new QPushButton("Open", this);
    removeBtn = new QPushButton("Remove", this);

    shuffleBtn   = new QPushButton(this);
    prevBtn      = new QPushButton(this);
    playPauseBtn = new QPushButton(this);
    nextBtn      = new QPushButton(this);
    repeatBtn    = new QPushButton(this);

    shuffleBtn->setIcon(QIcon(":/icons/shuffle.svg"));
    prevBtn->setIcon(QIcon(":/icons/previous.svg"));
    playPauseBtn->setIcon(QIcon(":/icons/play.svg"));
    nextBtn->setIcon(QIcon(":/icons/next.svg"));
    repeatBtn->setIcon(QIcon(":/icons/repeat.svg"));

    shuffleBtn->setToolTip("Enable Shuffle");
    prevBtn->setToolTip("Previous");
    playPauseBtn->setToolTip("Play");
    nextBtn->setToolTip("Next");
    repeatBtn->setToolTip("Enable Repeat");
    repeatBtn->setCheckable(true);

    shuffleBtn->setCheckable(true);
    shuffleBtn->setStyleSheet(R"(
        QPushButton:checked {
            color: #1DB954;
        }
    )");

    for (auto* btn : {shuffleBtn, prevBtn, playPauseBtn, nextBtn, repeatBtn}) {
        btn->setFlat(true);        // Spotify-like
        btn->setIconSize(QSize(24, 24));
        btn->setCursor(Qt::PointingHandCursor);
    }

    // Right-side controls
    QVBoxLayout* controls = new QVBoxLayout;
    controls->addWidget(openBtn);
    controls->addWidget(removeBtn);
    controls->addStretch();

    // Main layout (Spotify-style)
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(playlistView, 3);  // left panel
    mainLayout->addLayout(controls, 1);       // right panel

    QHBoxLayout* transportBar = new QHBoxLayout;
    transportBar->addStretch();
    transportBar->addWidget(shuffleBtn);
    transportBar->addWidget(prevBtn);
    transportBar->addWidget(playPauseBtn);
    transportBar->addWidget(nextBtn);
    transportBar->addWidget(repeatBtn);
    transportBar->addStretch();

    QString transportStyle = R"(
        QPushButton {
            border: none;
            background: transparent;
            padding: 6px;
            border-radius: 16px;
        }

        QPushButton:hover {
            background-color: rgba(255, 255, 255, 30);
        }

        QPushButton:pressed {
            background-color: rgba(255, 255, 255, 60);
        }

        QPushButton:checked {
            background-color: rgba(29, 185, 84, 40);
        }
    )";

    for (auto* btn : {shuffleBtn, prevBtn, playPauseBtn, nextBtn, repeatBtn}) {
        btn->setStyleSheet(transportStyle);
    }

    progressSlider = new QSlider(Qt::Horizontal, this);
    progressSlider->setRange(0, 0); // duration unknown initially
    progressSlider->setEnabled(false);

    timeLabel = new QLabel("0:00 / 0:00", this);
    timeLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout* bottomBar = new QVBoxLayout;
    bottomBar->addLayout(transportBar);
    bottomBar->addWidget(progressSlider);
    bottomBar->addWidget(timeLabel);
    QVBoxLayout* rootLayout = new QVBoxLayout;
    rootLayout->addLayout(mainLayout, 1);
    rootLayout->addLayout(bottomBar, 0);

    setLayout(rootLayout);
}

void MainWindow::connectSignals()
{
    connect(openBtn, &QPushButton::clicked,
            this, &MainWindow::addTrackFromFile);

    connect(playPauseBtn, &QPushButton::clicked, this, [this]() {
        if (isPlaying) {
            player.pause();
            playPauseBtn->setIcon(QIcon(":/icons/play.svg"));
            playPauseBtn->setToolTip("Play");
        } else {
            player.play();
            playPauseBtn->setIcon(QIcon(":/icons/pause.svg"));
            playPauseBtn->setToolTip("Pause");
        }
        isPlaying = !isPlaying;
    });

    connect(nextBtn, &QPushButton::clicked, [this]() {
        if (playlist.empty()) return;
        playTrack(playlist.next());
    });

    connect(prevBtn, &QPushButton::clicked, [this]() {
        if (playlist.empty()) return;
        playTrack(playlist.prev());
    });

    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::removeSelectedTrack);

    connect(shuffleBtn, &QPushButton::toggled, this, [this](bool on) {
        // qDebug() << "Shuffle:" << (on ? "ON" : "OFF");
        if (on) {
            // Shuffle playlist with a random seed
            std::random_device rd;
            playlist.shuffle(rd());
            shuffleBtn->setToolTip("Disable Shuffle");
        } else {
            // Disable shuffle
            playlist.disableShuffle();
            shuffleBtn->setToolTip("Enable Shuffle");
        }
    });

    connect(repeatBtn, &QPushButton::clicked, this, [this]() {
        auto mode = playlist.getRepeatMode();

        switch (mode) {
        case PlaylistImpl::RepeatMode::Off:
            mode = PlaylistImpl::RepeatMode::All;
            repeatBtn->setChecked(true);
            repeatBtn->setIcon(QIcon(":/icons/repeat.svg"));   // normal repeat icon
            repeatBtn->setToolTip("Enable Repeat One");
            break;
        case PlaylistImpl::RepeatMode::All:
            mode = PlaylistImpl::RepeatMode::One;
            repeatBtn->setChecked(true);
            repeatBtn->setIcon(QIcon(":/icons/repeat-1.svg")); // repeat one icon
            repeatBtn->setToolTip("Disable Repeat");
            break;
        case PlaylistImpl::RepeatMode::One:
            mode = PlaylistImpl::RepeatMode::Off;
            repeatBtn->setChecked(false);
            repeatBtn->setIcon(QIcon(":/icons/repeat.svg"));   // normal icon but off
            repeatBtn->setToolTip("Enable Repeat");
            break;
        }

        playlist.setRepeatMode(mode);
    });

    connect(playlistView, &QTableWidget::cellDoubleClicked,
            [this](int row, int /*column*/) {
                if (row >= 0 && row < playlistView->rowCount())
                    playTrack(playlist.at(row));
            });

    connect(&player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            // Track finished, play next track
            if (!playlist.empty()) {
                Track nextTrack = playlist.next();
                playTrack(nextTrack);
            }
        }
    });

    connect(&player, &QMediaPlayer::durationChanged,
            this, [this](qint64 duration) {
                progressSlider->setRange(0, duration);
                progressSlider->setEnabled(duration > 0);
            });

    connect(&player, &QMediaPlayer::positionChanged,
            this, [this](qint64 position) {
                if (!progressSlider->isSliderDown())
                    progressSlider->setValue(position);

                qint64 dur = player.duration();
                auto formatTime = [](qint64 ms) {
                    int sec = ms / 1000;
                    return QString("%1:%2")
                        .arg(sec / 60)
                        .arg(sec % 60, 2, 10, QChar('0'));
                };

                timeLabel->setText(
                    formatTime(position) + " / " + formatTime(dur)
                    );
            });

    connect(progressSlider, &QSlider::sliderMoved,
            this, [this](int value) {
                player.setPosition(value);
            });
}

void MainWindow::addTrackFromFile()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Open Audio Files",
        "",
        "Audio Files (*.mp3 *.wav *.ogg)"
        );

    if (files.isEmpty())
        return;

    for (const QString& file : files) {
        Track t;
        t.filename = file.toStdString();

        // Get metadata
        Mp3Metadata data = Mp3Reader::read(t.filename);
        t.title  = data.title;
        t.artist = data.artist;
        t.album  = data.album;

        // Use temporary QMediaPlayer to get duration
        QMediaPlayer probePlayer;
        QAudioOutput audioOutput;
        probePlayer.setAudioOutput(&audioOutput);
        probePlayer.setSource(QUrl::fromLocalFile(file));

        QEventLoop loop;
        QObject::connect(&probePlayer, &QMediaPlayer::durationChanged, &loop, &QEventLoop::quit);
        loop.exec();

        t.lengthSeconds = probePlayer.duration() / 1000;

        // Add to playlist
        playlist.add(t);

        // Add row to table
        int row = playlistView->rowCount();
        playlistView->insertRow(row);
        auto* indexItem = new QTableWidgetItem(QString::number(row + 1));
        indexItem->setTextAlignment(Qt::AlignCenter);
        playlistView->setItem(row, 0, indexItem);
        playlistView->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(t.title)));
        playlistView->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(t.artist)));
        playlistView->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(t.album)));

        int minutes = t.lengthSeconds / 60;
        int seconds = t.lengthSeconds % 60;
        QString lenStr = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        auto* durationItem = new QTableWidgetItem(lenStr);
        durationItem->setTextAlignment(Qt::AlignCenter);
        playlistView->setItem(row, 4, durationItem);
    }

    // Start playing the first of the new selection
    if (!files.isEmpty()) {
        playTrack(playlist.at(currentIndex >= 0 ? currentIndex : 0));
        playPauseBtn->setIcon(QIcon(":/icons/pause.svg"));
    }
}

void MainWindow::playTrack(const Track& t)
{
    if (t.filename.empty())
        return;

    player.setSource(QUrl::fromLocalFile(QString::fromStdString(t.filename)));
    player.play();

    // Remove previous bold & indicator
    if (currentIndex >= 0 && currentIndex < playlistView->rowCount()) {
        for (int col = 0; col < playlistView->columnCount(); ++col)
            playlistView->item(currentIndex, col)->setFont(QFont());

        // Remove previous TrackIndicator if exists
        QWidget* w = playlistView->cellWidget(currentIndex, 0);
        if (w) {
            playlistView->removeCellWidget(currentIndex, 0);
            delete w;
            // Restore track number
            playlistView->setItem(currentIndex, 0, new QTableWidgetItem(QString::number(currentIndex + 1)));
        }
    }

    // Set new currentIndex
    for (int i = 0; i < playlistView->rowCount(); ++i) {
        if (playlist.at(i).filename == t.filename) {
            currentIndex = i;
            break;
        }
    }

    // Bold all columns of current row
    if (currentIndex >= 0) {
        QFont boldFont;
        boldFont.setBold(true);
        for (int col = 1; col < playlistView->columnCount(); ++col)
            playlistView->item(currentIndex, col)->setFont(boldFont);

        // Replace track number with animated indicator
        TrackIndicator* indicator = new TrackIndicator;
        playlistView->setCellWidget(currentIndex, 0, indicator);
        indicator->start();

        playlistView->selectRow(currentIndex);
        playlistView->scrollToItem(playlistView->item(currentIndex, 1));
    }
    progressSlider->setValue(0);
    timeLabel->setText("0:00 / 0:00");
}

void MainWindow::removeSelectedTrack()
{
    int row = playlistView->currentRow();
    if (row < 0) return;

    bool removingCurrent = (row == currentIndex);

    if (removingCurrent) {
        player.stop();
    }

    playlist.removeAt(row);
    playlistView->removeRow(row);

    if (playlistView->rowCount() == 0) {
        currentIndex = -1;
        updateTrackNumbers();
        return;
    }

    if (row < currentIndex) {
        currentIndex--;
    }

    updateTrackNumbers();

    if (removingCurrent) {
        int next = qMin(row, playlistView->rowCount() - 1);
        playTrack(playlist.at(next));
    }
}

void MainWindow::updateTrackNumbers()
{
    for (int row = 0; row < playlistView->rowCount(); ++row) {

        // Remove any existing widget (indicator or stale widget)
        QWidget* w = playlistView->cellWidget(row, 0);
        if (w) {
            playlistView->removeCellWidget(row, 0);
            delete w;
        }

        auto* item = new QTableWidgetItem(QString::number(row + 1));
        item->setTextAlignment(Qt::AlignCenter);
        playlistView->setItem(row, 0, item);
    }
}
