#pragma once

#include <string>

struct Track {
    std::string filename;
    std::string title;
    std::string artist;
    std::string album;
    int lengthSeconds; // total seconds
};

class Playlist {
public:
    virtual ~Playlist() = default;

    virtual Track at(size_t index) const = 0;
    virtual void add(const Track& track) = 0;
    virtual Track next() = 0;
    virtual Track prev() = 0;
    virtual bool empty() const = 0;
};
