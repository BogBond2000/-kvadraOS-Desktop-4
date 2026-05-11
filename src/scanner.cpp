#include "scanner.hpp"

#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stack>

namespace fs = std::filesystem;

const std::unordered_set<std::string> Scanner::AUDIO_EXT = {
    "mp3", "wav", "flac", "ogg", "aac", "m4a", "wma", "opus",
    "aiff", "aif", "ape", "wv", "mka", "ra", "ram", "mid",
    "midi", "amr", "dts", "ac3"
};

const std::unordered_set<std::string> Scanner::VIDEO_EXT = {
    "mp4", "mpg", "mpeg", "avi", "mkv", "mov", "webm", "wmv",
    "flv", "f4v", "m4v", "3gp", "3g2", "ogv", "ts", "m2ts",
    "mts", "vob", "rm", "rmvb", "divx", "asf"
};

const std::unordered_set<std::string> Scanner::IMAGE_EXT = {
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "tiff", "tif",
    "svg", "ico", "heic", "heif", "raw", "cr2", "nef", "arw",
    "dng", "psd", "xcf", "avif", "jxl", "pbm", "pgm", "ppm"
};



Scanner::Scanner(const std::string& directory)
    : m_directory(directory)
{}

std::string Scanner::toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Scanner::getExtension(const std::string& filename) {
    auto pos = filename.rfind('.');
    if (pos == std::string::npos) return "";
    return filename.substr(pos + 1);
}

MediaFiles Scanner::scan() const {
    MediaFiles result;

    if (!fs::exists(m_directory) || !fs::is_directory(m_directory)) {
        std::cerr << "[Scanner] Path does not exist or is not a directory: " << m_directory << std::endl;
        return result;
    }

    std::stack<fs::path> dirs;
    dirs.push(m_directory);

    while (!dirs.empty()) {
        fs::path currentDir = dirs.top();
        dirs.pop();

        try {
            for (const auto& entry : fs::directory_iterator(currentDir, fs::directory_options::skip_permission_denied)) {
                try {
                    if (entry.is_symlink()) {
                        continue;
                    }
                    if (entry.is_regular_file()) {
                        const std::string filename = entry.path().filename().string();
                        const std::string ext = toLower(getExtension(filename));

                        if (!ext.empty()) {
                            if (AUDIO_EXT.count(ext)) {
                                result.audio.push_back(filename);
                            } else if (VIDEO_EXT.count(ext)) {
                                result.video.push_back(filename);
                            } else if (IMAGE_EXT.count(ext)) {
                                result.images.push_back(filename);
                            }
                        }
                    } else if (entry.is_directory()) {
                        dirs.push(entry.path());
                    }
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Warning: " << e.what() << std::endl;
                }
            }
        } catch (const fs::filesystem_error& e) {

            std::cerr << "Warning: cannot open directory " << currentDir << ": " << e.what() << std::endl;
        }
    }

    std::sort(result.audio.begin(),  result.audio.end());
    std::sort(result.video.begin(),  result.video.end());
    std::sort(result.images.begin(), result.images.end());

    return result;
}