#include <iostream>

#ifdef _WIN32
// Windows: CLI version not supported
int run_cli() {
    std::cerr << "CLI mode not supported on Windows." << std::endl;
    return 0;
}
#else
#include <vlc/vlc.h>
#include <ncurses.h>
#include "PlaylistImpl.h"
#include "Mp3Reader.h"
#include <vector>
#include <string>
#include <memory>
#include <random>

int run_cli() {
    PlaylistImpl playlist;

    std::vector<std::string> files = {
        "media/Adele_-_Hello.mp3",
        "media/Daniel_Powter_-_Freeloop.mp3",
        "media/Miley_Cyrus_-_Party_In_The_USA.mp3",
        "media/award.mp3",
        "media/part1.mp3",
        "media/tellmewhy.mp3",
        "media/谢霆锋-因为爱所以爱.mp3"
    };

    // Load tracks with metadata
    for (const auto& file : files) {
        Track t;
        t.filename = file;
        Mp3Metadata data = Mp3Reader::read(file);
        t.title  = data.title.empty() ? file : data.title;
        t.artist = data.artist;
        t.album  = data.album;
        t.lengthSeconds = data.lengthSeconds;
        playlist.add(t);
    }

    if (playlist.empty()) return 0;

    initscr();
    timeout(100); // Wait 100ms for input, then continue the loop anyway
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    Track currentTrack = playlist.at(0);

    // Initialize libVLC with increased buffers
    const char* vlc_args[] = {
        "--aout=pulse",           // Force PulseAudio
        "--file-caching=5000",    // Buffer 3 seconds of file data
        "--network-caching=5000", // Buffer 3 seconds of network data
        "--clock-jitter=2000",     // Increase jitter tolerance
        "--clock-synchro=0",       // Disable strict clock sync
        "--no-interact" // Prevents popups from crashing your terminal
    };

    libvlc_instance_t* vlc = libvlc_new(6, vlc_args);

    // CRITICAL: Always check for NULL to avoid Segmentation Faults
    if (!vlc) {
        endwin();
        std::cerr << "Failed to initialize libVLC. Check arguments." << std::endl;
        return 1;
    }

    libvlc_media_player_t* player = nullptr;

    auto playTrack = [&](const Track& t) {
        if (player) {
            libvlc_media_player_stop(player);
            libvlc_media_player_release(player);
        }

        libvlc_media_t* media = libvlc_media_new_path(vlc, t.filename.c_str());
        player = libvlc_media_player_new_from_media(media);
        libvlc_media_release(media);
        libvlc_media_player_play(player);
    };

    playTrack(currentTrack);

    auto draw_ui = [&](WINDOW* win) {
        werase(win);
        mvwprintw(win, 0, 0, "Terminal Music Player (n: next, p: prev, r: repeat, s: shuffle, q: quit)");
        mvwprintw(win, 1, 0, "-------------------------------------------------------------------------------");
        mvwprintw(win, 2, 0, "%3s  %-30s %-20s %-20s %6s", "#", "Title", "Artist", "Album", "Time");

        for (int i = 0; i < playlist.size(); ++i) {
            Track t = playlist.at(i);
            if (t.filename == currentTrack.filename)
                wattron(win, A_BOLD | A_REVERSE);

            int min = t.lengthSeconds / 60;
            int sec = t.lengthSeconds % 60;
            char timeBuf[16];
            snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", min, sec);

            mvwprintw(win, i + 3, 0, "%3d  %-30s %-20s %-20s %6s",
                      i + 1,
                      t.title.c_str(),
                      t.artist.c_str(),
                      t.album.c_str(),
                      timeBuf);

            if (t.filename == currentTrack.filename)
                wattroff(win, A_BOLD | A_REVERSE);
        }
        // ADD THIS: Get current state
        libvlc_state_t state = player ? libvlc_media_player_get_state(player) : libvlc_NothingSpecial;
        const char* stateStr = "Unknown";
        if (state == libvlc_Playing) stateStr = "PLAYING";
        else if (state == libvlc_Buffering) stateStr = "BUFFERING...";
        else if (state == libvlc_Error) stateStr = "ERROR";

        mvwprintw(win, playlist.size() + 4, 0, "Status: [%s]", stateStr);
        wrefresh(win);
    };

    draw_ui(stdscr);

    bool isRunning = true;
    while (isRunning) {
        int ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case 'n':
                    currentTrack = playlist.next();
                    playTrack(currentTrack);
                    break;
                case 'p':
                    currentTrack = playlist.prev();
                    playTrack(currentTrack);
                    break;
                case 'r':
                    if (playlist.getRepeatMode() == PlaylistImpl::RepeatMode::Off)
                        playlist.setRepeatMode(PlaylistImpl::RepeatMode::All);
                    else if (playlist.getRepeatMode() == PlaylistImpl::RepeatMode::All)
                        playlist.setRepeatMode(PlaylistImpl::RepeatMode::One);
                    else
                        playlist.setRepeatMode(PlaylistImpl::RepeatMode::Off);
                    break;
                case 's':
                    if (playlist.shuffled())
                        playlist.disableShuffle();
                    else
                        playlist.shuffle(std::random_device{}());
                    break;
                case 'q':
                    isRunning = false;
                    break;
            }
        }

        draw_ui(stdscr);
    }

    if (player) {
        libvlc_media_player_stop(player);
        libvlc_media_player_release(player);
    }
    libvlc_release(vlc);
    endwin();

    return 0;
}
#endif
