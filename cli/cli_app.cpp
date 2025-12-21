#include <iostream>
#include "PlaylistImpl.h"

int run_cli()
{
    PlaylistImpl playlist;

    playlist.add({"media/tellmewhy.mp3", "Song 1"});
    playlist.add({"media/part1.mp3", "Song 2"});

    std::cout << "Commands: n (next), p (prev), q (quit)\n";

    char cmd;
    while (std::cin >> cmd) {
        if (cmd == 'n')
            std::cout << "Now: " << playlist.next().title << "\n";
        else if (cmd == 'p')
            std::cout << "Now: " << playlist.prev().title << "\n";
        else if (cmd == 'q')
            break;
    }

    return 0;
}
