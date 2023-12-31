#pragma once
#include <errno.h>
#include <mysql/mysql.h>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <webserver/buffer.h>
#include <webserver/sqlconnRAII.h>
#include <webserver/sqlconnpool.h>

namespace webserver {
class HttpRequest {
public:
  enum PARSE_STATE {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH,
  };

  enum HTTP_CODE {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
  };

  HttpRequest() { init(); }
  ~HttpRequest() = default;

  void init();
  bool parse(Buffer &buff);

  std::string path() const;
  std::string &path();
  std::string method() const;
  std::string version() const;
  std::string getpost(const std::string &key) const;
  std::string getpost(const char *key) const;

  bool is_keepalive() const;


private:
  bool parse_requestline(const std::string &line);
  void parse_header(const std::string &line);
  void parse_body(const std::string &line);

  void parse_path();
  void parse_post();
  void parse_fromurlencoded();

  static bool userverify(const std::string &name, const std::string &pwd, bool isLogin);

  PARSE_STATE m_state;
  std::string m_method, m_path, m_version, m_body;
  std::unordered_map<std::string, std::string> m_header;
  std::unordered_map<std::string, std::string> m_post;

  static const std::unordered_set<std::string> DEFAULT_HTML;
  static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
  static int converhex(char ch);
};

} // namespace webserver