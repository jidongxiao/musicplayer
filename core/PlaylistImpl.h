#pragma once

#include "Playlist.h"
#include <vector>

class PlaylistImpl : public Playlist {
public:
    enum class RepeatMode { Off, All, One };
    PlaylistImpl();

    void add(const Track& track) override;
    Track next() override;
    Track prev() override;
    bool empty() const override;
    Track at(size_t index) const override;
    void removeAt(size_t index);
    void rebuildPlaybackOrder();

    bool shuffled(){ return isShuffled; }
    void shuffle(unsigned int seed);
    void disableShuffle();
    void setRepeatMode(RepeatMode mode) { repeatMode = mode; }
    RepeatMode getRepeatMode() const { return repeatMode; }

    size_t size() const { return tracks.size(); }

private:
    std::vector<Track> tracks;
    std::vector<size_t> playbackOrder;
    int current;
    bool isShuffled;
    RepeatMode repeatMode;
};
