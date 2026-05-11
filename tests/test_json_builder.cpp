#include <gtest/gtest.h>
#include "json_builder.hpp"

#include <string>

static bool has(const std::string& json, const std::string& substr) {
    return json.find(substr) != std::string::npos;
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: базовая структура JSON
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, ContainsAllKeys) {
    MediaFiles files;
    std::string json = JsonBuilder::build(files);

    EXPECT_TRUE(has(json, "\"audio\""));
    EXPECT_TRUE(has(json, "\"video\""));
    EXPECT_TRUE(has(json, "\"images\""));
}

TEST(JsonBuilderTest, StartsWithBrace) {
    MediaFiles files;
    std::string json = JsonBuilder::build(files);

    auto pos = json.find_first_not_of(" \t\r\n");
    ASSERT_NE(pos, std::string::npos);
    EXPECT_EQ(json[pos], '{');
}

TEST(JsonBuilderTest, EndsWithBrace) {
    MediaFiles files;
    std::string json = JsonBuilder::build(files);

    auto pos = json.find_last_not_of(" \t\r\n");
    ASSERT_NE(pos, std::string::npos);
    EXPECT_EQ(json[pos], '}');
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: пустые списки
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, EmptyMediaFiles) {
    MediaFiles files;
    std::string json = JsonBuilder::build(files);

    EXPECT_TRUE(has(json, "[]"));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: содержимое аудио
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, AudioFilesPresent) {
    MediaFiles files;
    files.audio = {"song.mp3", "track.wav"};

    std::string json = JsonBuilder::build(files);

    EXPECT_TRUE(has(json, "\"song.mp3\""));
    EXPECT_TRUE(has(json, "\"track.wav\""));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: содержимое видео
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, VideoFilesPresent) {
    MediaFiles files;
    files.video = {"movie.mkv", "clip.mp4"};

    std::string json = JsonBuilder::build(files);

    EXPECT_TRUE(has(json, "\"movie.mkv\""));
    EXPECT_TRUE(has(json, "\"clip.mp4\""));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: содержимое изображений
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, ImageFilesPresent) {
    MediaFiles files;
    files.images = {"photo.jpg", "logo.png"};

    std::string json = JsonBuilder::build(files);

    EXPECT_TRUE(has(json, "\"photo.jpg\""));
    EXPECT_TRUE(has(json, "\"logo.png\""));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: все три категории вместе
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, AllCategoriesTogether) {
    MediaFiles files;
    files.audio  = {"111.mp3", "222.wav"};
    files.video  = {"333.mpg"};
    files.images = {"444.jpeg", "555.png"};

    std::string json = JsonBuilder::build(files);

    EXPECT_TRUE(has(json, "\"audio\""));
    EXPECT_TRUE(has(json, "\"video\""));
    EXPECT_TRUE(has(json, "\"images\""));

    EXPECT_TRUE(has(json, "\"111.mp3\""));
    EXPECT_TRUE(has(json, "\"222.wav\""));
    EXPECT_TRUE(has(json, "\"333.mpg\""));
    EXPECT_TRUE(has(json, "\"444.jpeg\""));
    EXPECT_TRUE(has(json, "\"555.png\""));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: порядок ключей (audio -> video -> images)
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, KeyOrder) {
    MediaFiles files;
    files.audio  = {"a.mp3"};
    files.video  = {"b.mp4"};
    files.images = {"c.jpg"};

    std::string json = JsonBuilder::build(files);

    auto pos_audio  = json.find("\"audio\"");
    auto pos_video  = json.find("\"video\"");
    auto pos_images = json.find("\"images\"");

    ASSERT_NE(pos_audio,  std::string::npos);
    ASSERT_NE(pos_video,  std::string::npos);
    ASSERT_NE(pos_images, std::string::npos);

    EXPECT_LT(pos_audio,  pos_video);
    EXPECT_LT(pos_video,  pos_images);
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: специальные символы в имени файла экранируются
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, SpecialCharsInFilename) {
    MediaFiles files;
    files.audio = {"my song (2024).mp3"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "my song (2024).mp3"));
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: один файл — без trailing запятой в массиве
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, SingleFileNoTrailingComma) {
    MediaFiles files;
    files.audio = {"only.mp3"};

    std::string json = JsonBuilder::build(files);

    auto item_pos = json.find("\"only.mp3\"");
    ASSERT_NE(item_pos, std::string::npos);

    auto bracket_pos = json.find(']', item_pos);
    ASSERT_NE(bracket_pos, std::string::npos);

    std::string between = json.substr(item_pos + 10, bracket_pos - item_pos - 10);
    EXPECT_EQ(between.find(','), std::string::npos);
}

// ════════════════════════════════════════════════════════════════════════════
//  Группа: безопасная сериализация — экранирование спецсимволов
// ════════════════════════════════════════════════════════════════════════════
TEST(JsonBuilderTest, EscapesDoubleQuotes) {
    MediaFiles files;
    files.audio = {"my \"song\".mp3"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "my \\\"song\\\".mp3"));
    EXPECT_EQ(json.find("\"my \"song\""), std::string::npos);
}

TEST(JsonBuilderTest, EscapesBackslash) {
    MediaFiles files;
    files.audio = {"path\\to\\file.mp3"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "path\\\\to\\\\file.mp3"));
}

TEST(JsonBuilderTest, EscapesNewline) {
    MediaFiles files;
    files.images = {"weird\nname.jpg"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "weird\\nname.jpg"));
}

TEST(JsonBuilderTest, EscapesTab) {
    MediaFiles files;
    files.images = {"weird\tname.jpg"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "weird\\tname.jpg"));
}

TEST(JsonBuilderTest, EscapesControlCharacters) {
    MediaFiles files;
    files.audio = {std::string("bad") + char(0x01) + "name.mp3"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "\\u0001"));
}

TEST(JsonBuilderTest, NormalFilenamesUnchanged) {
    MediaFiles files;
    files.audio  = {"normal_song-2024 (1).mp3"};
    files.images = {"фото.jpg"};

    std::string json = JsonBuilder::build(files);
    EXPECT_TRUE(has(json, "normal_song-2024 (1).mp3"));
    EXPECT_TRUE(has(json, "фото.jpg"));
}

