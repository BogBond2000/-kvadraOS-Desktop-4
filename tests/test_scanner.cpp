#include <gtest/gtest.h>
#include "scanner.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() {
        m_path = fs::temp_directory_path() / ("media_watcher_test_" + std::to_string(::getpid()));
        fs::create_directories(m_path);
    }
    ~TempDir() {
        fs::remove_all(m_path);
    }

    void createFile(const std::string& relative_path) {
        auto full = m_path / relative_path;
        fs::create_directories(full.parent_path());
        std::ofstream(full).flush();
    }

    fs::path path() const { return m_path; }
    std::string str()  const { return m_path.string(); }

private:
    fs::path m_path;
};

static bool contains(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: пустая директория
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, EmptyDirectory) {
    TempDir tmp;
    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_TRUE(result.audio.empty());
    EXPECT_TRUE(result.video.empty());
    EXPECT_TRUE(result.images.empty());
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: несуществующая директория
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, NonExistentDirectory) {
    Scanner scanner("/tmp/this_path_does_not_exist_12345678");
    MediaFiles result = scanner.scan();

    EXPECT_TRUE(result.audio.empty());
    EXPECT_TRUE(result.video.empty());
    EXPECT_TRUE(result.images.empty());
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: аудио файлы
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, AudioExtensions) {
    TempDir tmp;
    const std::vector<std::string> audio_files = {
        "song.mp3", "track.wav", "music.flac",
        "beat.aac", "loop.ogg", "album.m4a", "old.wma"
    };
    for (const auto& f : audio_files) tmp.createFile(f);

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(), audio_files.size());
    EXPECT_TRUE(result.video.empty());
    EXPECT_TRUE(result.images.empty());

    for (const auto& f : audio_files) {
        EXPECT_TRUE(contains(result.audio, f)) << "Missing: " << f;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: видео файлы
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, VideoExtensions) {
    TempDir tmp;
    const std::vector<std::string> video_files = {
        "film.mp4", "show.mkv", "clip.avi", "movie.mov",
        "old.mpg",  "encode.mpeg", "win.wmv", "stream.flv", "web.webm"
    };
    for (const auto& f : video_files) tmp.createFile(f);

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.video.size(), video_files.size());
    EXPECT_TRUE(result.audio.empty());
    EXPECT_TRUE(result.images.empty());

    for (const auto& f : video_files) {
        EXPECT_TRUE(contains(result.video, f)) << "Missing: " << f;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: файлы изображений
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, ImageExtensions) {
    TempDir tmp;
    const std::vector<std::string> image_files = {
        "photo.jpg", "scan.jpeg", "logo.png", "anim.gif",
        "icon.bmp",  "web.webp",  "raw.tiff", "raw2.tif", "vector.svg"
    };
    for (const auto& f : image_files) tmp.createFile(f);

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.images.size(), image_files.size());
    EXPECT_TRUE(result.audio.empty());
    EXPECT_TRUE(result.video.empty());

    for (const auto& f : image_files) {
        EXPECT_TRUE(contains(result.images, f)) << "Missing: " << f;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: смешанные файлы
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, MixedFiles) {
    TempDir tmp;
    tmp.createFile("audio.mp3");
    tmp.createFile("audio.wav");
    tmp.createFile("video.mp4");
    tmp.createFile("image.jpg");
    tmp.createFile("image.png");
    tmp.createFile("document.pdf");
    tmp.createFile("archive.zip");
    tmp.createFile("readme.txt");

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(),  2u);
    EXPECT_EQ(result.video.size(),  1u);
    EXPECT_EQ(result.images.size(), 2u);

    EXPECT_TRUE(contains(result.audio,  "audio.mp3"));
    EXPECT_TRUE(contains(result.audio,  "audio.wav"));
    EXPECT_TRUE(contains(result.video,  "video.mp4"));
    EXPECT_TRUE(contains(result.images, "image.jpg"));
    EXPECT_TRUE(contains(result.images, "image.png"));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: файлы БЕЗ расширения игнорируются
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, FilesWithoutExtensionIgnored) {
    TempDir tmp;
    tmp.createFile("Makefile");
    tmp.createFile("README");
    tmp.createFile("mp3");
    tmp.createFile("song.mp3");

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(), 1u);
    EXPECT_TRUE(contains(result.audio, "song.mp3"));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: регистр расширения не важен (MP3 == mp3)
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, CaseInsensitiveExtensions) {
    TempDir tmp;
    tmp.createFile("SONG.MP3");
    tmp.createFile("PHOTO.JPG");
    tmp.createFile("FILM.MP4");
    tmp.createFile("track.Mp3");
    tmp.createFile("image.Png");

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(),  2u);
    EXPECT_EQ(result.video.size(),  1u);
    EXPECT_EQ(result.images.size(), 2u);
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: рекурсивный обход поддиректорий
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, RecursiveSubdirectories) {
    TempDir tmp;
    tmp.createFile("root.mp3");
    tmp.createFile("Music/album/track.wav");
    tmp.createFile("Videos/movie.mkv");
    tmp.createFile("Photos/2024/January/photo.jpg");
    tmp.createFile("Photos/2024/February/scan.png");

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(),  2u);
    EXPECT_EQ(result.video.size(),  1u);
    EXPECT_EQ(result.images.size(), 2u);

    EXPECT_TRUE(contains(result.audio,  "root.mp3"));
    EXPECT_TRUE(contains(result.audio,  "track.wav"));
    EXPECT_TRUE(contains(result.video,  "movie.mkv"));
    EXPECT_TRUE(contains(result.images, "photo.jpg"));
    EXPECT_TRUE(contains(result.images, "scan.png"));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: результат отсортирован
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, ResultIsSorted) {
    TempDir tmp;
    tmp.createFile("z_song.mp3");
    tmp.createFile("a_song.mp3");
    tmp.createFile("m_song.mp3");

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    ASSERT_EQ(result.audio.size(), 3u);
    EXPECT_EQ(result.audio[0], "a_song.mp3");
    EXPECT_EQ(result.audio[1], "m_song.mp3");
    EXPECT_EQ(result.audio[2], "z_song.mp3");
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: большое количество файлов
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, LargeNumberOfFiles) {
    TempDir tmp;
    const int N = 500;

    for (int i = 0; i < N; ++i) {
        tmp.createFile("audio_" + std::to_string(i) + ".mp3");
        tmp.createFile("image_" + std::to_string(i) + ".jpg");
        tmp.createFile("video_" + std::to_string(i) + ".mp4");
    }

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(),  static_cast<size_t>(N));
    EXPECT_EQ(result.images.size(), static_cast<size_t>(N));
    EXPECT_EQ(result.video.size(),  static_cast<size_t>(N));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: директории не считаются файлами
// ════════════════════════════════════════════════════════════════════════════
TEST(ScannerTest, DirectoriesAreNotCounted) {
    TempDir tmp;
    fs::create_directories(tmp.path() / "fake.mp3");
    fs::create_directories(tmp.path() / "fake.jpg");
    tmp.createFile("real.mp3");

    Scanner scanner(tmp.str());
    MediaFiles result = scanner.scan();

    EXPECT_EQ(result.audio.size(), 1u);
    EXPECT_TRUE(contains(result.audio, "real.mp3"));
    EXPECT_TRUE(result.images.empty());
}
