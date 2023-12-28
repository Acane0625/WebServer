#include <webserver/buffer.h>

namespace webserver {

char *Buffer::beginptr() {
  return &*m_buffer.begin();
}

const char *Buffer::beginptr() const {
  return &*m_buffer.begin();
}

void Buffer::makespace(std::size_t len) {
  if(writeable_bytes() + prependable_bytes() < len) {
    m_buffer.resize(m_writepos + len + 1);
  } else {
    size_t readable = readable_bytes();
    std::copy(beginptr() + m_readpos, beginptr() + m_writepos, beginptr());
    m_readpos = 0;
    m_writepos = m_readpos + readable;
    assert(readable == readable_bytes());
  }
}

size_t Buffer::writeable_bytes() const {
  return m_buffer.size() - m_writepos;
}

size_t Buffer::readable_bytes() const {
  return m_writepos - m_readpos;
}

size_t Buffer::prependable_bytes() const {
  return m_readpos;
}

const char *Buffer::peek() const {
  return beginptr() + m_readpos;
}

void Buffer::ensure_writeable(size_t len) {
  if(writeable_bytes() < len) makespace(len);
  assert(writeable_bytes() >= len);
}

void Buffer::has_written(size_t len) {
  m_writepos += len;
}

void Buffer::retrieve(size_t len) {
  assert(len <= readable_bytes());
  m_readpos += len;
}

void Buffer::retrieve_until(const char *end) {
  assert(peek() <= end);
  retrieve(end - peek());
}

void Buffer::retrieveall() {
  m_buffer.assign(m_buffer.size(), 0);
  m_readpos = m_writepos = 0;
}

std::string Buffer::retrieveall_to_str() {
  std::string str(peek(), readable_bytes());
  retrieveall();
  return str;
}

const char *Buffer::beginwrite_const() const {
  return beginptr() + m_writepos;
}

char *Buffer::beginwrite() {
  return beginptr() + m_writepos;
}

void Buffer::append(const std::string &str) {
  append(str.data(), str.size());
}

void Buffer::append(const char *str, size_t len) {
  assert(str);
  ensure_writeable(len);
  std::copy(str, str + len, beginwrite());
  has_written(len);
}

void Buffer::append(const void *data, size_t len) {
  assert(data);
  append(static_cast<const char*>(data), len);
}

void Buffer::append(const Buffer &other) {
  append(other.peek(), other.m_readpos);
}

ssize_t Buffer::readfd(int fd, int *Errno) {
  char temp[65535];
  iovec iov[2];
  const size_t writeable = writeable_bytes();
  iov[0] = {beginptr() + writeable, writeable};
  iov[1] = {temp, sizeof(temp)};
  const ssize_t len = readv(fd, iov, 2);
  if(len < 0) *Errno = errno;
  else if(static_cast<size_t>(len) <= writeable) m_writepos += len;
  else {
    m_writepos = m_buffer.size();
    append(temp, len - writeable);
  }
  return len;
}

ssize_t Buffer::writefd(int fd, int *Errno) {
  size_t readable = readable_bytes();
  ssize_t len = write(fd, peek(), readable);
  if(len < 0) {
    *Errno = errno;
    return len;
  }
  m_readpos += len;
  return len;
}
  
} // namespace webserver