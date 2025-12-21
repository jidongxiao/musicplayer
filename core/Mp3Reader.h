#ifndef MP3READER_H
#define MP3READER_H

#include <string>
#include <fstream>
#include <array>
#include <algorithm>

struct Mp3Metadata {
    std::string title;
    std::string artist;
    std::string album;
    int lengthSeconds; // optional, can compute separately
};

class Mp3Reader {
public:
    static Mp3Metadata read(const std::string& filename);
};

#endif // MP3READER_H
