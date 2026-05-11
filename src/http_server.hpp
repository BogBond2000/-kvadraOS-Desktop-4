#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

using JsonProvider = std::function<std::string()>;

class HttpServer final{
public:
    HttpServer(int port, JsonProvider provider);
    ~HttpServer();

    HttpServer(const HttpServer&)            = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&)                 = delete;
    HttpServer& operator=(HttpServer&&)      = delete;

    void start();
    void stop();

private:
    int                  m_port;
    std::atomic<int>     m_server_fd{-1};
    JsonProvider         m_provider;
    std::thread          m_thread;
    std::atomic<bool>    m_running{false};

    std::mutex              m_clients_mutex;
    std::vector<std::thread> m_client_threads;

    void run();
    void handleClient(int client_fd);
    void reapFinishedThreads();
    static std::string buildResponse(int status_code,
                                     const std::string& status_text,
                                     const std::string& body);

    static std::string parsePath(const std::string& request_line);

    static bool readRequest(int fd, std::string& out_request);
};
