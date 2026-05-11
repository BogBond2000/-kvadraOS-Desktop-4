#pragma once

#include <unistd.h>
#include <utility>

class FdHandle final{
public:
    static constexpr int INVALID_FD = -1;

    explicit FdHandle(int fd = INVALID_FD) noexcept
        : m_fd(fd)
    {}

    FdHandle(const FdHandle&)            = delete;
    FdHandle& operator=(const FdHandle&) = delete;

    FdHandle(FdHandle&& other) noexcept
        : m_fd(other.m_fd)
    {
        other.m_fd = INVALID_FD;
    }

    FdHandle& operator=(FdHandle&& other) noexcept {
        if (this != &other) {
            close();
            m_fd       = other.m_fd;
            other.m_fd = INVALID_FD;
        }
        return *this;
    }

    ~FdHandle() {
        close();
    }

    bool valid() const noexcept { return m_fd != INVALID_FD; }
    explicit operator bool() const noexcept { return valid(); }

    int get() const noexcept { return m_fd; }

    void close() noexcept {
        if (valid()) {
            ::close(m_fd);
            m_fd = INVALID_FD;
        }
    }

    int release() noexcept {
        int fd = m_fd;
        m_fd   = INVALID_FD;
        return fd;
    }

private:
    int m_fd;
};
