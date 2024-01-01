#include <webserver/httpresponse.h>

namespace webserver {
const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse()
    : m_code(-1), m_path(""), m_srcdir(""), is_keepalive(false),
      mmfile(nullptr) {
  mmfilestat = {0};
}

HttpResponse::~HttpResponse() { unmapfile(); }

void HttpResponse::init(const std::string &srcDir, std::string &path,
                        bool isKeepAlive, int code) {
  assert(srcDir != "");
  if (mmfile)
    unmapfile();
  m_code = code;
  is_keepalive = isKeepAlive;
  m_path = path;
  m_srcdir = srcDir;
  mmfile = nullptr;
  mmfilestat = {0};
}

void HttpResponse::makeresponse(Buffer &buff) {
  if (stat((m_srcdir + m_path).data(), &mmfilestat) < 0 ||
      S_ISDIR(mmfilestat.st_mode)) {
    m_code = 404;
  } else if (!(mmfilestat.st_mode & S_IROTH)) {
    m_code = 403;
  } else if (m_code == -1) {
    m_code = 200;
  }
  errorhtml();
  add_stateline(buff);
  add_header(buff);
  add_content(buff);
}

void HttpResponse::unmapfile() {
  if (mmfile) {
    munmap(mmfile, mmfilestat.st_size);
    mmfile = nullptr;
  }
}

char *HttpResponse::file() { return mmfile; }

size_t HttpResponse::filelen() const { return mmfilestat.st_size; }

void HttpResponse::error_content(Buffer &buff, std::string message) {
  std::string body;
  std::string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (CODE_STATUS.count(m_code))
    status = CODE_STATUS.find(m_code)->second;
  else
    status = "Bad Request";
  body += std::to_string(m_code) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";
  buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
  buff.append(body);
}

void HttpResponse::add_stateline(Buffer &buff) {
  std::string status;
  if (CODE_STATUS.count(m_code)) {
    status = CODE_STATUS.find(m_code)->second;
  } else {
    m_code = 400;
    status = CODE_STATUS.find(400)->second;
  }
  buff.append("HTTP/1.1 " + std::to_string(m_code) + " " + status + "\r\n");
}

void HttpResponse::add_header(Buffer &buff) {
  buff.append("Connection: ");
  if (is_keepalive) {
    buff.append("keep-alive\r\n");
    buff.append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.append("close\r\n");
  }
  buff.append("Content-type: " + getfiletype() + "\r\n");
}

void HttpResponse::add_content(Buffer &buff) {
  int srcfd = open((m_srcdir + m_path).data(), O_RDONLY);
  if (srcfd < 0) {
    error_content(buff, "File NotFound!");
    return;
  }
  // LOG_DEBUG("file path %s", (srcDir_ + path_).data());
  int *mmRet =
      (int *)mmap(0, mmfilestat.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
  if (*mmRet == -1) {
    error_content(buff, "File NotFound!");
    return;
  }
  mmfile = (char *)mmRet;
  close(srcfd);
  buff.append("Content-length: " + std::to_string(mmfilestat.st_size) +
              "\r\n\r\n");
}

void HttpResponse::errorhtml() {
  if (CODE_PATH.count(m_code)) {
    m_path = CODE_PATH.find(m_code)->second;
    stat((m_srcdir + m_path).data(), &mmfilestat);
  }
}

std::string HttpResponse::getfiletype() {
  std::string::size_type idx = m_path.find_last_of('.');
  if (idx == std::string::npos)
    return "text/plain";
  std::string suf = m_path.substr(idx);
  if (SUFFIX_TYPE.count(suf))
    return SUFFIX_TYPE.find(suf)->second;
  return "text/plain";
}
} // namespace webserver