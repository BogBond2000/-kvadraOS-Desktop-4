#include "scanner.hpp"
#include "json_builder.hpp"
#include "http_server.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>

#include <pwd.h>
#include <unistd.h>

static std::atomic<bool> g_running{true};

void signalHandler(int) {
    g_running = false;
}

static std::string getHomeDir() {
    const char* home = std::getenv("HOME");
    if (home && *home != '\0') return home;
    struct passwd* pw = ::getpwuid(::getuid());
    if (pw) return pw->pw_dir;
    return ".";
}

static bool writeAtomic(const std::string& path, const std::string& content) {
    const std::string tmp_path = path + ".tmp";

    std::ofstream ofs(tmp_path, std::ios::trunc);
    if (!ofs) {
        std::cerr << "[Scanner] Cannot open for writing: " << tmp_path << "\n";
        return false;
    }

    ofs << content;
    ofs.flush();

    if (!ofs.good()) {
        std::cerr << "[Scanner] Write failed (disk full?): " << tmp_path << "\n";
        return false;
    }
    ofs.close();

    if (std::rename(tmp_path.c_str(), path.c_str()) != 0) {
        std::cerr << "[Scanner] rename() failed: " << strerror(errno) << "\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::string scan_path = getHomeDir();
    int interval  = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--path" || arg == "-p") && i + 1 < argc) {
            scan_path = argv[++i];
        } else if ((arg == "--interval" || arg == "-i") && i + 1 < argc) {
            interval = std::stoi(argv[++i]);
        }
    }

    const std::string output_file = getHomeDir() + "/.media_files";

    std::mutex  json_mutex;
    std::string json_cache = "{}\n";

    HttpServer server(1234, [&]() -> std::string {
        std::lock_guard<std::mutex> lock(json_mutex);
        return json_cache;
    });
    server.start();

    Scanner scanner(scan_path);

    while (g_running) {
        try {
            MediaFiles  files = scanner.scan();
            std::string json  = JsonBuilder::build(files);

            std::cout << "[Scanner] Found: "
                      << files.audio.size()  << " audio, "
                      << files.video.size()  << " video, "
                      << files.images.size() << " images\n";

            {
                std::lock_guard<std::mutex> lock(json_mutex);
                json_cache = json;
            }

            if (writeAtomic(output_file, json)) {
                std::cout << "[Scanner] Written to " << output_file << "\n";
            }

        } catch (const std::exception& e) {
            std::cerr << "[Error] " << e.what() << "\n";
        }

        for (int i = 0; i < interval && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }


    server.stop();
    return 0;
}
