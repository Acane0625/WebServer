#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <webserver/buffer.h>

namespace webserver {
class HttpResponse {
public:
  HttpResponse();
  ~HttpResponse();
  void init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);
  void makeresponse(Buffer &buff);
  void unmapfile();
  char *file();
  size_t filelen() const;
  void error_content(Buffer &buff, std::string message);
  int code() const { return m_code; }

private:
  void add_stateline(Buffer &buff);
  void add_header(Buffer &buff);
  void add_content(Buffer &buff);

  void errorhtml();
  std::string getfiletype();

  int m_code;
  bool is_keepalive;

  std::string m_path;
  std::string m_srcdir;

  char *mmfile;
  struct stat mmfilestat;

  static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
  static const std::unordered_map<int, std::string> CODE_STATUS;
  static const std::unordered_map<int, std::string> CODE_PATH;
};
} // namespace webserver