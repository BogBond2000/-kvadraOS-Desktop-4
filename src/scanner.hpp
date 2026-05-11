#pragma once

#include <string>
#include <vector>
#include <unordered_set>

struct MediaFiles {
    std::vector<std::string> audio;
    std::vector<std::string> video;
    std::vector<std::string> images;
};

class Scanner final {
public:
    explicit Scanner(const std::string& directory);
    MediaFiles scan() const;

private:
    std::string m_directory;

    static const std::unordered_set<std::string> AUDIO_EXT;
    static const std::unordered_set<std::string> VIDEO_EXT;
    static const std::unordered_set<std::string> IMAGE_EXT;

    static std::string toLower(const std::string& s);
    static std::string getExtension(const std::string& filename);
};
