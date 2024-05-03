
#include "buffer.h"

Buffer::Buffer(int InitBufferSize)
    : buffer_(InitBufferSize), readPos_(0), writePos_(0) {}

size_t Buffer::WritableBytes() const
{
    return buffer_.size() - Buffer::writePos_;
}
size_t Buffer::ReadableBytes() const
{
    return writePos_ - readPos_;
}
size_t Buffer::PrependableBytes() const
{
    return readPos_;
}

const char *Buffer::Peek() const
{
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWriteable(size_t len)
{
    if (WritableBytes() < len)
        MakeSpace_(len);
    // 写下断言确保分配成功
    assert(WritableBytes() >= len);
}
void Buffer::HasWritten(size_t len)
{
    writePos_ = writePos_ + len;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    readPos_ = readPos_ + len;
}
void Buffer::RetrieveUntil(const char *end)
{
    // assert(end -Peek() <= ReadableBytes());  //原项目有这个，但是这里应该是不用的,因为我们可以判断可读和可写的具体数据在内存中的位置。
    // readPos_ += end -Peek();
    assert(Peek() <= end);
    Retrieve(end - Peek());
}
void Buffer::RetrieveAll()
{
    // bzero(&buffer_[0], buffer_.size());
    readPos_ = writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr()
{
    // std::string str(Peek(), writePos_ - readPos_);
    //  readPos_ = writePos_ = 0;
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char *Buffer::BeginWriteConst() const
{
    return BeginPtr_() + writePos_;
}
char *Buffer::BeginWrite()
{
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
    // c_str() 函数返回的指针可以确保指向以 null 结尾的字符串，适用于传递给需要以 C 风格字符串为参数的函数，而 data() 函数返回的指针可能不以 null 结尾，适用于需要访问字符串数据但不需要 null 结尾的场景。
    // 一般来说data() 比 c_str() 更快，因为data()不需要创建副本，在buffer这种追求性能的场景中，我们使用data();
}

void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const void *data, size_t len)
{
    // std::copy(static_cast<const char *>(data),Peek(), len);
    //  HasWritten(len);
    assert(data);
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const Buffer &buff)
{
    // std::copy(buff.Peek() + readPos_,Peek(),buff.writePos_ - buff.readPos_);
    // HasWritten(buff.writePos_ - buff.readPos_);
    Append(buff.Peek(), buff.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int *SaveErrno) // 从socket读取数据
{
    char buff[65535];
    struct iovec iov[2]; // iovec 是一个在 POSIX 兼容系统中的结构体，用于在一次系统调用中读写多个非连续的内存区域。
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = &buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len <= 0)
    {
        *SaveErrno = errno; // 保存错误码，防止错误码被覆盖
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        HasWritten(len);
    }
    else
    {
        writePos_ = buffer_.size();
        Buffer::Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int *SaveErrno) // 写入数据socket
{

    ssize_t len = write(fd, Peek(), ReadableBytes());
    if (len <= 0)
    {
        *SaveErrno = errno; // 保存错误码，防止错误码被覆盖
        return len;
    }
    Retrieve(len);
    return len;
}

char *Buffer::BeginPtr_()
{
    return &*buffer_.begin();
}
const char *Buffer::BeginPtr_() const
{
    return &*buffer_.begin();
}
void Buffer::MakeSpace_(size_t len)
{ // 分为移动readable数据能找到空闲空间和一定需要分配空间两种情况
    if (WritableBytes() + PrependableBytes() < len)
    {
        buffer_.resize(writePos_ + len + 1); // vector从0开始，要＋1
    }
    else
    {
        size_t readable = ReadableBytes();
        std::copy(Peek(), Peek() + readable, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}
// std::vector<char> buffer_;
// std::atomic<std::size_t> readPos_;
// std::atomic<std::size_t> writePos_;
