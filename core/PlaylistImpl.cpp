#include "PlaylistImpl.h"

#include <numeric>   // For std::iota
#include <random>    // For std::mt19937
#include <algorithm> // For std::shuffle

PlaylistImpl::PlaylistImpl()
    : current(-1),
    isShuffled(false),
    repeatMode(RepeatMode::Off)
{
}

void PlaylistImpl::add(const Track& track)
{
    tracks.push_back(track);
    rebuildPlaybackOrder();

    if (current == -1)
        current = 0;
}

Track PlaylistImpl::next()
{
    if (tracks.empty())
        return {};

    if (repeatMode == RepeatMode::One) {
        // Keep the current track
        return tracks[playbackOrder[current]];
    }

    current = (current + 1) % playbackOrder.size();

    if (current == 0 && repeatMode == RepeatMode::Off) {
        // Reached end and repeat off → stop
        current = playbackOrder.size() - 1; // keep last track
    }

    return tracks[playbackOrder[current]];
}

Track PlaylistImpl::prev()
{
    if (tracks.empty())
        return {};

    if (repeatMode == RepeatMode::One) {
        // Keep the current track
        return tracks[playbackOrder[current]];
    }

    current--;

    if (current < 0) {
        if (repeatMode == RepeatMode::All) {
            // wrap around to last track
            current = playbackOrder.size() - 1;
        } else {
            // repeat off → stay at first track
            current = 0;
        }
    }

    return tracks[playbackOrder[current]];
}

bool PlaylistImpl::empty() const
{
    return tracks.empty();
}

Track PlaylistImpl::at(size_t index) const
{
    if (index >= tracks.size())
        return {};
    return tracks[index];
}

void PlaylistImpl::removeAt(size_t index)
{
    if (index >= tracks.size())
        return;

    tracks.erase(tracks.begin() + index);

    // Adjust current index if needed
    if (tracks.empty()) {
        current = -1;
    } else if (current >= tracks.size()) {
        current = tracks.size() - 1;
    }
}

void PlaylistImpl::rebuildPlaybackOrder()
{
    playbackOrder.resize(tracks.size());
    std::iota(playbackOrder.begin(), playbackOrder.end(), 0);
}

void PlaylistImpl::shuffle(unsigned int seed)
{
    rebuildPlaybackOrder();

    std::mt19937 g(seed);
    std::shuffle(playbackOrder.begin(), playbackOrder.end(), g);

    current = 0;
    isShuffled = true;
}

void PlaylistImpl::disableShuffle()
{
    isShuffled = false;
    rebuildPlaybackOrder();
    current = 0;
}
