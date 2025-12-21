#pragma once

#include "Playlist.h"
#include <vector>

class PlaylistImpl : public Playlist {
public:
    PlaylistImpl();

    void add(const Track& track) override;
    Track next() override;
    Track prev() override;
    bool empty() const override;
    Track at(size_t index) const override;
    void removeAt(size_t index);

private:
    std::vector<Track> tracks;
    int current;
};
