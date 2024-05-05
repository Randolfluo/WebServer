
#include "log.h"


void Log::init(int level, const char *path, const char *suffix,
               int maxQueueCapacity) {
  isOpen_ = true;
  level_ = level;
  //处理是同步日志还是异步日志
  if (maxQueueCapacity > 0) {
    isAsync_ = true;
    if (!deque_) {
      std::unique_ptr<BlockDeque<std::string>> newDequeue(
          new BlockDeque<std::string>);
      deque_ = move(newDequeue);

      std::unique_ptr<std::thread> newThread(new std::thread(FlushLogThread));
      writeThread_ = move(newThread);
    }
  } else {
    isAsync_ = false;
  }

  lineCount_ = 0;
  time_t timer = std::time(nullptr);
  struct tm *sysTime = localtime(&timer);
  struct tm t = *sysTime;
  path_ = path;
  suffix_ = suffix;

  char fileName[LOG_NAME_LEN] = {0};
  snprintf(fileName, LOG_NAME_LEN, "%s/%04d_%02d_%02d%s", path_,
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  toDay_ = t.tm_mday;

  {
    std::lock_guard<std::mutex> locker(mtx_);
    buff_.RetrieveAll(); //清空缓冲区
    if (fp_)             //关闭上次未关闭的文件描述符
    {
      flush();
      fclose(fp_);
    }

    fp_ = fopen(fileName, "a");
    if (fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(fileName, "a");
    }

    assert(fp_ != nullptr);
  }
}


Log *Log::Instance() {
  static Log inst;
  return &inst;
}

void Log::FlushLogThread() {
  //切换为同步模式写完日志
  Log::Instance()->AsyncWrite_();
}

void Log::write(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  //通过将 now 初始化为 {0, 0}，表示当前时间被设置为 Epoch（1970-01-01 00:00:00
  //UTC）。
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm *systime = localtime(&tSec);
  struct tm t = *systime;

  va_list vaList;

  //若日志日期或日志长度超过，我们需要重建文件
  if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
    std::unique_lock<std::mutex> locker(mtx_);
    locker.unlock();
    char newfileName[LOG_NAME_LEN];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
             t.tm_mday);

    if (toDay_ != t.tm_mday) {
      snprintf(newfileName, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
      toDay_ = t.tm_mday;
      lineCount_ = 0;
    } else {
      snprintf(newfileName, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail,
               (lineCount_ / MAX_LINES), suffix_);
    }

    locker.lock();
    flush();
    fclose(fp_);
    fp_ = fopen(newfileName, "a");
    assert(fp_ != nullptr);
  }

  {
    std::unique_lock<std::mutex> locker(mtx_);
    lineCount_++;
    int n =
        snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
                 t.tm_sec, now.tv_usec);
    buff_.HasWritten(n);
    AppendLogLevelTitle_(level_);

    va_start(vaList, format);
    int m =
        vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
    va_end(vaList);
    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);
    if (isAsync_ && deque_ && !deque_->full()) {
      deque_->push_back(buff_.RetrieveAllToStr());
    } else {
       fputs(buff_.Peek(), fp_); 
    
    }
    buff_.RetrieveAll();
  }

  // TODO这里频繁开关文件的原因
  //及时更新日志，防止webserver意外终止时日志丢失
}
void Log::flush() {
  if (isAsync_) {
    deque_->flush();
  }
  fflush(fp_); //清空流
}

int Log::GetLevel() {
  std::unique_lock<std::mutex> locker(mtx_);
  return level_;
}
void Log::SetLevel(int level) {
  std::unique_lock<std::mutex> locker(mtx_);
  level_ = level;
}

// bool IsOpen() { return isOpen_; }

Log::Log() {
  lineCount_ = 0;
  toDay_ = 0;
  isAsync_ = false;
  fp_ = nullptr;
  deque_ = nullptr;
  writeThread_ = nullptr;
}

void Log::AppendLogLevelTitle_(int level) {

  switch (level) {
  case 0:
    buff_.Append("[debug]: ", 9);
    break;
  case 1:
    buff_.Append("[info] : ", 9);
    break;
  case 2:
    buff_.Append("[warn] : ", 9);
    break;
  case 3:
    buff_.Append("[error]: ", 9);
    break;
  default:
    buff_.Append("[info] : ", 9);
    break;
  }
}

Log::~Log() {
  if (writeThread_ && writeThread_->joinable()) {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!deque_->empty()) {
      deque_->flush();
    }
    deque_->Close();
    writeThread_->join();
  }
  if (fp_) {
    std::lock_guard<std::mutex> locker(mtx_);
    flush();
    fclose(fp_);
  }
}
void Log::AsyncWrite_() {
  std::string str = "";
  while (deque_->pop(str)) {
    std::lock_guard<std::mutex> locker(mtx_);
    fputs(str.c_str(), fp_);
  }
}
