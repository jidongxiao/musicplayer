#include "PlaylistImpl.h"

PlaylistImpl::PlaylistImpl()
    : current(-1)
{
}

void PlaylistImpl::add(const Track& track)
{
    tracks.push_back(track);

    if (current == -1)
        current = 0;
}

Track PlaylistImpl::next()
{
    if (tracks.empty())
        return {};

    current = (current + 1) % tracks.size();
    return tracks[current];
}

Track PlaylistImpl::prev()
{
    if (tracks.empty())
        return {};

    current = (current - 1 + tracks.size()) % tracks.size();
    return tracks[current];
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
