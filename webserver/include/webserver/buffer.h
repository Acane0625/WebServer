#pragma once
#include <cstring>
#include <vector>
#include <atomic>
#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>
namespace webserver {
  class Buffer {
  private:
    std::vector<char> m_buffer;
    std::atomic<std::size_t> m_readpos;
    std::atomic<std::size_t> m_writepos;

    char* beginptr();
    const char* beginptr() const;
    void makespace(std::size_t len);

  public:
    Buffer(int sz = 1024) : m_buffer(sz), m_readpos(0), m_writepos(0) {}
    size_t writeable_bytes() const;
    size_t readable_bytes() const;
    size_t prependable_bytes() const;

    const char* peek() const;
    void ensure_writeable(size_t len);
    void has_written(size_t len);

    void retrieve(size_t len);
    void retrieve_until(const char* end);
    void retrieveall();
    std::string retrieveall_to_str();

    const char* beginwrite_const() const;
    char* beginwrite();

    void append(const std::string &str);
    void append(const char* str, size_t len);
    void append(const void* data, size_t len);
    void append(const Buffer& other);

    ssize_t readfd(int fd, int* Errno);
    ssize_t writefd(int fd, int* Errno);
  };
};