#include "http_server.hpp"
#include "fd_handle.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

HttpServer::HttpServer(int port, JsonProvider provider)
    : m_port(port)
    , m_provider(std::move(provider))
{}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    m_running = true;
    m_thread  = std::thread(&HttpServer::run, this);
}

void HttpServer::stop() {
    m_running = false;

    int fd = m_server_fd.exchange(-1);
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::vector<std::thread> threads;
    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        threads = std::move(m_client_threads);
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

void HttpServer::reapFinishedThreads() {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    m_client_threads.erase(
        std::remove_if(m_client_threads.begin(), m_client_threads.end(),
            [](std::thread& t) {
                return !t.joinable();
            }),
        m_client_threads.end()
    );
}

void HttpServer::run() {
    FdHandle server(::socket(AF_INET, SOCK_STREAM, 0));
    if (!server) {
        std::cerr << "[HTTP] socket() failed: " << strerror(errno) << std::endl;
        return;
    }

    int opt = 1;
    ::setsockopt(server.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(m_port);

    if (::bind(server.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[HTTP] bind() failed: " << strerror(errno) << std::endl;
        return;
    }

    if (::listen(server.get(), 16) < 0) {
        std::cerr << "[HTTP] listen() failed: " << strerror(errno) << std::endl;
        return;
    }

    m_server_fd.store(server.get());
    std::cout << "[HTTP] Listening on port " << m_port << std::endl;

    while (m_running) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        FdHandle client(::accept(server.get(),
                                 reinterpret_cast<sockaddr*>(&client_addr),
                                 &client_len));
        if (!client) {
            if (m_running) {
                std::cerr << "[HTTP] accept() failed: " << strerror(errno) << std::endl;
            }
            continue;
        }

        reapFinishedThreads();

        int raw_fd = client.release();
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_client_threads.emplace_back([this, raw_fd]() {
            FdHandle conn(raw_fd);
            handleClient(conn.get());
        });
    }

    m_server_fd.store(-1);
}

bool HttpServer::readRequest(int fd, std::string& out_request) {
    char buf[4096];
    out_request.clear();
    out_request.reserve(1024);

    while (true) {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) {
            return !out_request.empty();
        }

        out_request.append(buf, static_cast<size_t>(n));

        if (out_request.find("\r\n\r\n") != std::string::npos) {
            return true;
        }

        if (out_request.size() > 1024 * 1024) {
            return false;
        }
    }
}

void HttpServer::handleClient(int client_fd) {
    std::string request;
    if (!readRequest(client_fd, request)) {
        std::string body     = "{\"error\": \"Bad Request\"}\n";
        std::string response = buildResponse(400, "Bad Request", body);
        ::send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::string first_line;
    auto newline = request.find("\r\n");
    if (newline != std::string::npos) {
        first_line = request.substr(0, newline);
    }

    const std::string path = parsePath(first_line);
    std::string response;

    if (path == "/media_files") {
        std::string json = m_provider();
        response = buildResponse(200, "OK", json);
    } else {
        response = buildResponse(404, "Not Found", "{\"error\": \"Not Found\"}\n");
    }

    const char* ptr  = response.c_str();
    size_t      left = response.size();
    while (left > 0) {
        ssize_t sent = ::send(client_fd, ptr, left, 0);
        if (sent < 0) {
            if (errno == EINTR) continue;
            break;
        }
        ptr  += sent;
        left -= static_cast<size_t>(sent);
    }
}

std::string HttpServer::buildResponse(int status_code,
                                      const std::string& status_text,
                                      const std::string& body)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    return oss.str();
}

std::string HttpServer::parsePath(const std::string& request_line) {
    auto first_space = request_line.find(' ');
    if (first_space == std::string::npos) return "/";
    auto second_space = request_line.find(' ', first_space + 1);
    if (second_space == std::string::npos) return "/";
    return request_line.substr(first_space + 1, second_space - first_space - 1);
}
