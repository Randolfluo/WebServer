

#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <cstddef>  //包括常用类型和宏的定义，比如 size_t 和 NULL。
#include <string>
#include <unordered_map>
#include <fcntl.h>      //于提供文件控制操作
#include <unistd.h> //POSIX操作系统的API
#include <sys/stat.h>   //文件状态和属性操作的结构和函数原型
#include <sys/mman.h>   //提供了内存映射文件的操作原型

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse{
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlice = false, int code = -1);
    void MakeResponse(Buffer& buff);
    void UnmapFile();
    char* File();
    size_t FileLen() const;
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const;

private:
    void AddStateLine_(Buffer &buff);
    void AddHeader_(Buffer &buff);
    void AddContent_(Buffer&buff);

    void ErrorHtml_();
    std::string GetFileType_();

private:
    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char* mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int , std::string> CODE_PATH;
};


#endif

