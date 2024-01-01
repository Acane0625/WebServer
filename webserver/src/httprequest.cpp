#include <webserver/httprequest.h>

namespace webserver {

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::init() {
  m_method = m_path = m_version = m_body = "";
  m_state = REQUEST_LINE;
  m_header.clear();
  m_post.clear();
}

bool HttpRequest::parse(Buffer &buff) {
  const char CRLF[] = "\r\n";
  if (buff.readable_bytes() < 0)
    return false;
  while (buff.readable_bytes() && m_state != FINISH) {
    const char *lineEnd =
        std::search(buff.peek(), buff.beginwrite_const(), CRLF, CRLF + 2);
    std::string line(buff.peek(), lineEnd);
    switch (m_state) {
    case REQUEST_LINE:
      if (!parse_requestline(line))
        return false;
      parse_path();
      break;
    case HEADERS:
      parse_header(line);
      if (buff.readable_bytes() <= 2)
        m_state = FINISH;
      break;
    case BODY:
      parse_body(line);
      break;
    default:
      break;
    }
    if (lineEnd == buff.beginwrite())
      break;
    buff.retrieve_until(lineEnd + 2);
  }
  // LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(),
  // version_.c_str());
  return true;
}

std::string HttpRequest::path() const { return m_path; }

std::string &HttpRequest::path() { return m_path; }

std::string HttpRequest::method() const { return m_method; }

std::string HttpRequest::version() const { return m_version; }

std::string HttpRequest::getpost(const std::string &key) const {
  assert(key != "");
  if (m_post.count(key))
    return m_post.find(key)->second;
  return "";
}

std::string HttpRequest::getpost(const char *key) const {
  assert(key != nullptr);
  if (m_post.count(key))
    return m_post.find(key)->second;
  return "";
}

bool HttpRequest::is_keepalive() const {
  if (m_header.count("Connection")) {
    return m_header.find("Connection")->second == "keep-alive" &&
           m_version == "1.1";
  }
  return false;
}

bool HttpRequest::parse_requestline(const std::string &line) {
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch submatch;
  if (std::regex_match(line, submatch, patten)) {
    m_method = submatch[1];
    m_path = submatch[2];
    m_version = submatch[3];
    m_state = HEADERS;
    return true;
  }
  // LOG_ERROR("RequestLine Error");
  return false;
}

void HttpRequest::parse_header(const std::string &line) {
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch submatch;
  if (std::regex_match(line, submatch, patten))
    m_header[submatch[1]] = submatch[2];
  else
    m_state = BODY;
}

void HttpRequest::parse_body(const std::string &line) {
  m_body = line;
  parse_post();
  m_state = FINISH;
  // LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::parse_path() {
  if (m_path == "/")
    m_path = "/index.html";
  else {
    for (auto &&item : DEFAULT_HTML) {
      if (item == m_path) {
        m_path += ".html";
        break;
      }
    }
  }
}

void HttpRequest::parse_post() {
  if (m_method == "POST" &&
      m_header["Content-Type"] == "application/x-www-form-urlencoded") {
    parse_fromurlencoded();
    if (DEFAULT_HTML_TAG.count(m_path)) {
      int tag = DEFAULT_HTML_TAG.find(m_path)->second;
      // LOG_DEBUG("Tag:%d", tag);
      if (tag == 0 || tag == 1) {
        bool isLogin = (tag == 1);
        if (userverify(m_post["username"], m_post["password"], isLogin))
          m_path = "/welcome.html";
        else
          m_path = "/error.html";
      }
    }
  }
}

void HttpRequest::parse_fromurlencoded() {
  if (m_body.size() == 0)
    return;
  std::string key, value;
  int num = 0;
  int n = m_body.size();
  int i = 0, j = 0;
  for (; i < n; i++) {
    char ch = m_body[i];
    switch (ch) {
    case '=':
      key = m_body.substr(j, i - j);
      j = i + 1;
      break;
    case '+':
      m_body[i] = ' ';
      break;
    case '%':
      num = converhex(m_body[i + 1]) * 16 + converhex(m_body[i + 2]);
      m_body[i + 2] = num % 10 + '0';
      m_body[i + 1] = num / 10 + '0';
      i += 2;
      break;
    case '&':
      value = m_body.substr(j, i - j);
      j = i + 1;
      m_post[key] = value;
      // LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
      break;
    default:
      break;
    }
  }
  assert(j <= i);
  if (m_post.count(key) == 0 && j < i) {
    value = m_body.substr(j, i - j);
    m_post[key] = value;
  }
}

bool HttpRequest::userverify(const std::string &name, const std::string &pwd,
                             bool isLogin) {
  if (name == "" || pwd == "")
    return false;
  // LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  MYSQL *sql;
  SqlConnRAII t(&sql, &SqlConnPool::Instance());
  assert(sql);
  bool flag = false;
  unsigned int j = 0;
  char order[256] = {0};
  MYSQL_FIELD *fields = nullptr;
  MYSQL_RES *res = nullptr;
  if (!isLogin)
    flag = true;
  snprintf(order, 256,
           "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
           name.c_str());
  // LOG_DEBUG("%s", order)  ;
  if (mysql_query(sql, order)) {
    mysql_free_result(res);
    return false;
  }
  res = mysql_store_result(sql);
  j = mysql_num_fields(res);
  fields = mysql_fetch_field(res);
  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    // LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
    std::string password(row[1]);
    if (isLogin) {
      if (pwd == password)
        flag = true;
      else {
        flag = false;
        // LOG_DEBUG("pwd error!");
      }
    } else {
      flag = false;
      // LOG_DEBUG("user used!");
    }
  }
  mysql_free_result(res);
  if (!isLogin && flag) {
    // LOG_DEBUG("regirster!");
    memset(order, 0, sizeof(order));
    snprintf(order, 256,
             "INSERT INTO user(username, password) VALUES('%s','%s')",
             name.c_str(), pwd.c_str());
    // LOG_DEBUG( "%s", order);
    if (mysql_query(sql, order)) {
      // LOG_DEBUG( "Insert error!");
      flag = false;
    }
    flag = true;
  }
  // LOG_DEBUG( "UserVerify success!!");
  return flag;
}

int HttpRequest::converhex(char ch) {
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  return ch;
}

} // namespace webserver