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

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    audio.setVolume(0.5);
    player.setAudioOutput(&audio);

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
    playBtn  = new QPushButton("Play", this);
    pauseBtn = new QPushButton("Pause", this);
    nextBtn  = new QPushButton("Next", this);
    prevBtn  = new QPushButton("Prev", this);
    removeBtn = new QPushButton("Remove", this);

    // Right-side controls
    QVBoxLayout* controls = new QVBoxLayout;
    controls->addWidget(openBtn);
    controls->addWidget(playBtn);
    controls->addWidget(pauseBtn);
    controls->addWidget(prevBtn);
    controls->addWidget(nextBtn);
    controls->addWidget(removeBtn);
    controls->addStretch();

    // Main layout (Spotify-style)
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(playlistView, 3);  // left panel
    mainLayout->addLayout(controls, 1);       // right panel

    progressSlider = new QSlider(Qt::Horizontal, this);
    progressSlider->setRange(0, 0); // duration unknown initially
    progressSlider->setEnabled(false);

    timeLabel = new QLabel("0:00 / 0:00", this);
    timeLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout* bottomBar = new QVBoxLayout;
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

    connect(playBtn, &QPushButton::clicked,
            &player, &QMediaPlayer::play);

    connect(pauseBtn, &QPushButton::clicked,
            &player, &QMediaPlayer::pause);

    connect(nextBtn, &QPushButton::clicked, [this]() {
        if (playlist.empty()) return;
        playTrack(playlist.next());
    });

    connect(prevBtn, &QPushButton::clicked, [this]() {
        if (playlist.empty()) return;
        playTrack(playlist.prev());
    });

    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::removeSelectedTrack);

    connect(playlistView, &QTableWidget::cellDoubleClicked,
            [this](int row, int /*column*/) {
                if (row >= 0 && row < playlistView->rowCount())
                    playTrack(playlist.at(row));
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
    QString file = QFileDialog::getOpenFileName(
        this,
        "Open Audio File",
        "",
        "Audio Files (*.mp3 *.wav *.ogg)"
        );

    if (file.isEmpty())
        return;

    Track t;
    t.filename = file.toStdString();

    // Get metadata from Mp3Reader
    Mp3Metadata data = Mp3Reader::read(t.filename);
    t.title  = data.title;
    t.artist = data.artist;
    t.album  = data.album;

    // Use temporary QMediaPlayer to get actual duration
    QMediaPlayer probePlayer;
    QAudioOutput audioOutput;
    probePlayer.setAudioOutput(&audioOutput);
    probePlayer.setSource(QUrl::fromLocalFile(file));

    // Process events to ensure duration is available
    QEventLoop loop;
    QObject::connect(&probePlayer, &QMediaPlayer::durationChanged,
                     &loop, &QEventLoop::quit);
    loop.exec();

    t.lengthSeconds = probePlayer.duration() / 1000; // convert ms -> sec

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
    QString lenStr = QString("%1:%2")
                         .arg(minutes)
                         .arg(seconds, 2, 10, QChar('0'));
    auto* durationItem = new QTableWidgetItem(lenStr);
    durationItem->setTextAlignment(Qt::AlignCenter);
    playlistView->setItem(row, 4, durationItem);

    playTrack(t);
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
