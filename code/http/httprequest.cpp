

#include "httprequest.h"

HttpRequest::HttpRequest() { Init(); }

HttpRequest::~HttpRequest() = default;

void HttpRequest::Init() {
  method_ = path_ = version_ = body_ = "";
  state_ = PARSE_STATE::REQUEST_LINE;
  header_.clear();
  post_.clear();
}

bool HttpRequest::parse(Buffer &buff) {
  const char CRLF[] = "\r\n";
  if (buff.ReadableBytes() <= 0)
    return false;
  while (buff.ReadableBytes() && state_ != PARSE_STATE::FINISH) {
    const char *lineEnd =
        std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
    std::string line(buff.Peek(), lineEnd);
    switch (state_) {
    case PARSE_STATE::REQUEST_LINE:
      if (!ParesRequestLine_(line))
        return false;
      ParsePath_();
      break;
    case PARSE_STATE::HEADERS:
      ParseHeader_(line);
      if (buff.ReadableBytes() <= 2)
        state_ = PARSE_STATE::FINISH;
      //若只有CRLF，则没有BODY
      break;

    case PARSE_STATE::BODY:
      ParseBody_(line);
      break;

    default:
      break;
    }
    if (lineEnd == buff.BeginWriteConst())
      break;
    buff.RetrieveUntil(lineEnd + 2); //记得+2
  }
  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(),
            version_.c_str());
  return true;
}

std::string HttpRequest::path() const { return path_; }
std::string &HttpRequest::path() { return path_; }
std::string HttpRequest::method() const { return method_; }
std::string HttpRequest::version() const { return version_; }
std::string HttpRequest::GetPost(const std::string &key) const {
  assert(key != "");
  if (post_.count(key) == 1) //找的到
    return post_.find(key)->second;
  return "";
}
std::string HttpRequest::GetPost(const char *key) const {
  assert(key != nullptr);
  if (post_.count(key) == 1) {
    return post_.find(key)->second;
  }
  return "";
}

bool HttpRequest::IsKeepAlive() const {
  if (header_.count("Connection") == 1)
    return header_.find("Connection")->second == "keep-alive" &&
           version_ == "1.1";
  return false;
}

/*
 TODO
 void HttpConn::ParseFormData() {}
 void HttpConn::ParseJson() {}
 */

bool HttpRequest::ParesRequestLine_(const std::string &line) {
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch subSmatch;
  if (std::regex_match(line, subSmatch, patten)) {
    //注意 subSmatch[0];表示匹配的整个字符串
    method_ = subSmatch[1];
    path_ = subSmatch[2];
    version_ = subSmatch[3];
    state_ = PARSE_STATE::HEADERS;
    return true;
  }
  LOG_ERROR("RequestLine Error");
  return false;
}

void HttpRequest::ParseHeader_(const std::string &line) {
  std::regex pattern("^([^:]*): ?(.*)$");
  std::smatch subSmatch;
  if (std::regex_match(line, subSmatch, pattern)) {
    header_[subSmatch[1]] = subSmatch[2];
  } else {
    state_ = PARSE_STATE::BODY;
  }
}

void HttpRequest::ParseBody_(const std::string &line) {
  body_ = line;
  ParsePost_();
  state_ = PARSE_STATE::FINISH;
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::ParsePath_() {
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    for (auto &item : DeFALUT_HTML) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}
void HttpRequest::ParsePost_() {
  if (method_ == "POST" &&
      header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParseFromUrlencoded_();
    if (DEFALUT_HTML_TAG.count(path_)) {
      int tag = DEFALUT_HTML_TAG.find(path_)->second;
      if (tag == 0 || tag == 1) {
        bool isLogin = (tag == 1);
        if (UserVerify(post_["username"], post_["password"], isLogin)) {
          path_ = "/welcome.html";
        } else {
          path_ = "/error.html";
        }
      }
    }
  }
}
//&、=、+、%等，这些字符在URL编码中用于表示特殊意义
void HttpRequest::ParseFromUrlencoded_() {
  if (body_.size() == 0) {
    return;
  }

  std::string key, value;
  int num = 0;
  int n = body_.size();
  int i = 0, j = 0; //循环和unordered_map
  for (; i < n; i++) {
    switch (body_[i]) {
    case '=':
      key = body_.substr(j, i - j);
      j = i + 1;
      break;
    case '+':
      body_[i] = ' ';
      break;
    case '%':
      num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
      body_[i + 1] = num % 10 + '0';
      body_[i + 2] = num / 10 + '0';
      i += 2;
      break;
    case '&':
      value = body_.substr(j, i - j);
      j = i + 1;
      post_[key] = value;
      LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
      LOG_DEBUG(body_.c_str());
      break;
    default:
      break;
    }
  }
  assert(j < i);
  //处理最后一个键值对，因为最后不会有&
  if (post_.count(key) == 0 && j < i) {
    value = body_.substr(j, i - j);
    post_[key] = value;
  }
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd,
                             bool isLogin) {
  if (name == "" || pwd == "")
    return false;
  LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  MYSQL *sql;

  SqlConnRAII(&sql, SqlConnPool::Instance());
  assert(sql);

  bool flag = false;
  size_t j = 0;
  char order[256] = {0};

  MYSQL_FIELD *fileds = nullptr;
  MYSQL_RES *res = nullptr;

  if (!isLogin) {
    flag = true;
  }

  snprintf(order, sizeof(order),
           "select username, password from user WHERE username = '%s' LIMIT 1",
           name.c_str());
  LOG_DEBUG("%s", order);

  if (mysql_query(sql, order)) //非0成功
  {
    mysql_free_result(res);

  }
  res = mysql_store_result(sql);
  j = mysql_num_fields(res);
  fileds = mysql_fetch_fields(res);

  while (MYSQL_ROW row = mysql_fetch_row(res)) { //遍历
    LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
    std::string password(row[1]);
    if (isLogin) {
      if (password == pwd) {
        flag = true;
      } else {
        flag = false;
        LOG_DEBUG("pwd error!");
      }
    } else {
      flag = false;
      LOG_DEBUG("user used!");
    }
  }

  mysql_free_result(res);

  if (!isLogin && flag == true) {
    LOG_DEBUG("regirster!");
    bzero(order, 256);
    snprintf(order, 256,
             "INSERT INTO user(username, password) VALUES('%s','%s')",
             name.c_str(), pwd.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order)) {
      LOG_DEBUG("Insert error!");
      flag = false;
    }
    flag = true;
  }
  SqlConnPool::Instance()->FreeConn(sql);
  LOG_DEBUG("UserVerify success!!");
  return flag;
}

const std::unordered_set<std::string> HttpRequest::DeFALUT_HTML{
    "/index", "/register", "/login", "/welcome", "video", "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFALUT_HTML_TAG{
    {{"/register.html", 0}, {"/login.html", 1}},
};

int HttpRequest::ConverHex(char ch) {
  if (ch >= 'A' && ch <= 'F')
    ch = ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f')
    ch = ch - 'a' + 10;
  return ch;
}
