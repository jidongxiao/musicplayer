#include "Mp3Reader.h"
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <iostream>

// -----------------------------
// Helpers
// -----------------------------

// Convert 4-byte synchsafe int to normal int
static uint32_t synchsafeToInt(const std::array<unsigned char,4>& bytes) {
    return (bytes[0] << 21) | (bytes[1] << 14) | (bytes[2] << 7) | (bytes[3]);
}

// Remove nulls and trim spaces
static std::string cleanString(const std::string& s) {
    std::string out;
    for (char c : s) if (c != '\0') out += c;
    auto start = out.find_first_not_of(' ');
    auto end   = out.find_last_not_of(' ');
    if (start == std::string::npos) return "";
    return out.substr(start, end - start + 1);
}

// Minimal UTF-16 decoding (ASCII subset only)
static std::string decodeUTF16(const std::vector<char>& data) {
    if (data.size() < 2) return "";
    std::string out;
    bool isLE = false;
    size_t pos = 0;

    // BOM detection
    if ((unsigned char)data[0]==0xFF && (unsigned char)data[1]==0xFE) { isLE = true; pos=2; }
    else if ((unsigned char)data[0]==0xFE && (unsigned char)data[1]==0xFF) { isLE=false; pos=2; }

    for (; pos+1 < data.size(); pos+=2) {
        uint16_t c = isLE ? ((unsigned char)data[pos] | ((unsigned char)data[pos+1]<<8))
                          : (((unsigned char)data[pos]<<8) | (unsigned char)data[pos+1]);
        if (c < 128) out += (char)c;
    }
    return cleanString(out);
}

// -----------------------------
// Mp3Reader Implementation
// -----------------------------
Mp3Metadata Mp3Reader::read(const std::string& filename) {
    Mp3Metadata meta = {"Unknown Title", "Unknown Artist", "Unknown Album", 0};

    std::ifstream file(filename, std::ios::binary);
    if (!file) return meta;

    // -------- ID3v2 detection --------
    char header[10];
    file.read(header,10);
    if (file.gcount()==10 && header[0]=='I' && header[1]=='D' && header[2]=='3') {
        uint8_t version = header[3]; // 3=v2.3, 4=v2.4
        uint8_t flags   = header[5];
        std::array<unsigned char,4> sizeBytes = {
            (unsigned char)header[6], (unsigned char)header[7],
            (unsigned char)header[8], (unsigned char)header[9]
        };
        uint32_t tagSize = synchsafeToInt(sizeBytes);

        std::vector<char> tagData(tagSize);
        file.read(tagData.data(), tagSize);

        size_t pos = 0;
        // Skip extended header if present
        if (flags & 0x40 && tagData.size() >= 4) {
            uint32_t extHeaderSize = synchsafeToInt({
                (unsigned char)tagData[0], (unsigned char)tagData[1],
                (unsigned char)tagData[2], (unsigned char)tagData[3]
            });
            pos = (version==3) ? (extHeaderSize + 4) : extHeaderSize;
        }

        while (pos+10 <= tagData.size()) {
            if ((unsigned char)tagData[pos]==0) break; // padding

            std::string frameID(tagData.data()+pos,4);
            uint32_t frameSize = 0;
            if (version==3) { // v2.3
                frameSize = ((unsigned char)tagData[pos+4]<<24) |
                            ((unsigned char)tagData[pos+5]<<16) |
                            ((unsigned char)tagData[pos+6]<<8) |
                            ((unsigned char)tagData[pos+7]);
            } else { // v2.4
                frameSize = synchsafeToInt({
                    (unsigned char)tagData[pos+4], (unsigned char)tagData[pos+5],
                    (unsigned char)tagData[pos+6], (unsigned char)tagData[pos+7]
                });
            }

            if (frameSize==0 || pos+10+frameSize > tagData.size()) break;

            const char* dataPtr = tagData.data()+pos+10;
            std::string value;

            if (frameSize>1) {
                uint8_t encoding = (unsigned char)dataPtr[0];
                if (encoding==0) value = cleanString(std::string(dataPtr+1, frameSize-1));
                else if (encoding==1 || encoding==2) {
                    std::vector<char> content(dataPtr+1, dataPtr+frameSize);
                    value = decodeUTF16(content);
                }
            }

            if (frameID=="TIT2") meta.title  = value;
            else if (frameID=="TPE1") meta.artist = value;
            else if (frameID=="TALB") meta.album  = value;
            else if (frameID=="TLEN") {
                try { meta.lengthSeconds = std::stoi(value)/1000; } catch(...) { meta.lengthSeconds=0; }
            }

            pos += 10 + frameSize;
        }
    }

    // -------- ID3v1 fallback --------
    file.seekg(0, std::ios::end);
    std::streampos filesize = file.tellg();
    if (filesize >= 128) {
        file.seekg(-128, std::ios::end);
        char id3v1[128];
        file.read(id3v1,128);
        if (id3v1[0]=='T' && id3v1[1]=='A' && id3v1[2]=='G') {
            auto trim = [](const char* s,size_t len){
                std::string str(s,len);
                str.erase(std::find_if(str.rbegin(), str.rend(),
                    [](unsigned char ch){ return !std::isspace(ch)&&ch!='\0'; }).base(), str.end());
                return str;
            };
            if(meta.title=="Unknown Title")  meta.title  = cleanString(trim(&id3v1[3],30));
            if(meta.artist=="Unknown Artist") meta.artist = cleanString(trim(&id3v1[33],30));
            if(meta.album=="Unknown Album")   meta.album  = cleanString(trim(&id3v1[63],30));
        }
    }

    return meta;
}
