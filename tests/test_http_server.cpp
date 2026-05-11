#include <gtest/gtest.h>
#include "http_server.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>

namespace fs = std::filesystem;

struct HttpResponse {
    int         status_code = 0;
    std::string body;
    std::string raw;
};

static HttpResponse doGet(int port, const std::string& path) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_GE(sock, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    for (int attempt = 0; attempt < 10; ++attempt) {
        if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::string request =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";

    ::send(sock, request.c_str(), request.size(), 0);

    std::string raw;
    char buf[4096];
    ssize_t n;
    while ((n = ::recv(sock, buf, sizeof(buf), 0)) > 0) {
        raw.append(buf, n);
    }
    ::close(sock);

    HttpResponse resp;
    resp.raw = raw;

    auto first_crlf = raw.find("\r\n");
    if (first_crlf != std::string::npos) {
        std::string status_line = raw.substr(0, first_crlf);
        auto sp1 = status_line.find(' ');
        if (sp1 != std::string::npos) {
            resp.status_code = std::stoi(status_line.substr(sp1 + 1, 3));
        }
    }

    auto header_end = raw.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        resp.body = raw.substr(header_end + 4);
    }

    return resp;
}
class HttpServerTest : public ::testing::Test {
protected:
    static constexpr int TEST_PORT = 19876;

    std::string  json_data = "{\"audio\":[],\"video\":[],\"images\":[]}\n";
    HttpServer*  server    = nullptr;

    void SetUp() override {
        server = new HttpServer(TEST_PORT, [this]() { return json_data; });
        server->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        server->stop();
        delete server;
        server = nullptr;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  Базовые тесты
// ════════════════════════════════════════════════════════════════════════════
TEST_F(HttpServerTest, Returns200ForMediaFiles) {
    auto resp = doGet(TEST_PORT, "/media_files");
    EXPECT_EQ(resp.status_code, 200);
}

TEST_F(HttpServerTest, Returns404ForUnknownPath) {
    auto resp = doGet(TEST_PORT, "/unknown");
    EXPECT_EQ(resp.status_code, 404);
}

TEST_F(HttpServerTest, Returns404ForRoot) {
    auto resp = doGet(TEST_PORT, "/");
    EXPECT_EQ(resp.status_code, 404);
}

TEST_F(HttpServerTest, BodyContainsJson) {
    json_data = "{\"audio\":[\"a.mp3\"],\"video\":[],\"images\":[]}\n";
    auto resp = doGet(TEST_PORT, "/media_files");

    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body.find("a.mp3"), std::string::npos);
}

TEST_F(HttpServerTest, ResponseContainsContentTypeJson) {
    auto resp = doGet(TEST_PORT, "/media_files");
    EXPECT_NE(resp.raw.find("Content-Type: application/json"), std::string::npos);
}

TEST_F(HttpServerTest, ResponseContainsContentLength) {
    auto resp = doGet(TEST_PORT, "/media_files");
    EXPECT_NE(resp.raw.find("Content-Length:"), std::string::npos);
}

TEST_F(HttpServerTest, MultipleSequentialRequests) {
    for (int i = 0; i < 5; ++i) {
        auto resp = doGet(TEST_PORT, "/media_files");
        EXPECT_EQ(resp.status_code, 200) << "Failed on request #" << i;
    }
}

TEST_F(HttpServerTest, JsonDataUpdatesAreReflected) {
    json_data = "{\"audio\":[\"first.mp3\"],\"video\":[],\"images\":[]}\n";
    auto resp1 = doGet(TEST_PORT, "/media_files");
    EXPECT_NE(resp1.body.find("first.mp3"), std::string::npos);

    json_data = "{\"audio\":[\"second.mp3\"],\"video\":[],\"images\":[]}\n";
    auto resp2 = doGet(TEST_PORT, "/media_files");
    EXPECT_NE(resp2.body.find("second.mp3"), std::string::npos);
    EXPECT_EQ(resp2.body.find("first.mp3"),  std::string::npos);
}

// ════════════════════════════════════════════════════════════════════════════
//  Параллельные запросы
// ════════════════════════════════════════════════════════════════════════════
TEST_F(HttpServerTest, ConcurrentRequests) {
    constexpr int NUM_THREADS = 10;
    std::vector<int> results(NUM_THREADS, 0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            auto resp  = doGet(TEST_PORT, "/media_files");
            results[i] = resp.status_code;
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 0; i < NUM_THREADS; ++i) {
        EXPECT_EQ(results[i], 200) << "Thread " << i << " failed";
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  Большой JSON — проверяем цикл send()
// ════════════════════════════════════════════════════════════════════════════
TEST_F(HttpServerTest, LargeJsonResponse) {
    std::string large_json = "{\"audio\": [";
    for (int i = 0; i < 1000; ++i) {
        if (i > 0) large_json += ",";
        large_json += "\"track_" + std::to_string(i) + ".mp3\"";
    }
    large_json += "], \"video\": [], \"images\": []}\n";

    json_data = large_json;
    auto resp = doGet(TEST_PORT, "/media_files");

    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body.find("track_999.mp3"), std::string::npos);
}

// ════════════════════════════════════════════════════════════════════════════
//  Сервер остаётся живым после невалидного запроса
// ════════════════════════════════════════════════════════════════════════════
TEST_F(HttpServerTest, ServerSurvivesBadRequest) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(sock, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(TEST_PORT);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    const char* bad = "GARBAGE\r\n\r\n";
    ::send(sock, bad, strlen(bad), 0);

    char buf[1024] = {};
    ::recv(sock, buf, sizeof(buf) - 1, 0);
    ::close(sock);

    auto resp = doGet(TEST_PORT, "/media_files");
    EXPECT_EQ(resp.status_code, 200);
}

TEST(AtomicWriteTest, NoPartialFileVisible) {
    const std::string path = fs::temp_directory_path().string()
                             + "/atomic_write_test_" + std::to_string(::getpid());
    const std::string tmp  = path + ".tmp";

    {
        std::ofstream ofs(path);
        ASSERT_TRUE(ofs.is_open());
        ofs << "initial";
    }

    {
        std::ofstream ofs(tmp);
        ASSERT_TRUE(ofs.is_open());
        ofs << "updated content";
        ofs.flush();
        ASSERT_TRUE(ofs.good());
    }

    ASSERT_EQ(std::rename(tmp.c_str(), path.c_str()), 0);

    {
        std::ifstream ifs(path);
        ASSERT_TRUE(ifs.is_open());
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        EXPECT_EQ(content, "updated content");
    }

    EXPECT_FALSE(fs::exists(tmp));

    fs::remove(path);
}
