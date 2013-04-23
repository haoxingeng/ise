/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// 文件名称: ise_classes.cpp
// 功能描述: 通用基础类库
///////////////////////////////////////////////////////////////////////////////

#include "ise_classes.h"
#include "ise_sys_utils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 类型定义

union Int64Rec
{
    INT64 value;
    struct {
        INT32 lo;
        INT32 hi;
    } ints;
};

///////////////////////////////////////////////////////////////////////////////
// class Buffer

Buffer::Buffer()
{
    init();
}

//-----------------------------------------------------------------------------

Buffer::Buffer(const Buffer& src)
{
    init();
    assign(src);
}

//-----------------------------------------------------------------------------

Buffer::Buffer(int size)
{
    init();
    setSize(size);
}

//-----------------------------------------------------------------------------

Buffer::Buffer(const void *buffer, int size)
{
    init();
    assign(buffer, size);
}

//-----------------------------------------------------------------------------

Buffer::~Buffer()
{
    if (buffer_)
        free(buffer_);
}

//-----------------------------------------------------------------------------

Buffer& Buffer::operator = (const Buffer& rhs)
{
    if (this != &rhs)
        assign(rhs);
    return *this;
}

//-----------------------------------------------------------------------------
// 描述: 将 buffer 中的 size 个字节复制到 *this 中，并将大小设置为 size
//-----------------------------------------------------------------------------
void Buffer::assign(const void *buffer, int size)
{
    setSize(size);
    if (size_ > 0)
        memmove(buffer_, buffer, size_);
    verifyPosition();
}

//-----------------------------------------------------------------------------
// 描述: 设置缓存大小
// 参数:
//   size     - 新缓冲区大小
//   initZero - 若新缓冲区比旧缓冲区大，是否将多余的空间用'\0'填充
// 备注:
//   新的缓存会保留原有内容
//-----------------------------------------------------------------------------
void Buffer::setSize(int size, bool initZero)
{
    if (size <= 0)
    {
        if (buffer_) free(buffer_);
        buffer_ = NULL;
        size_ = 0;
        position_ = 0;
    }
    else if (size != size_)
    {
        void *newBuf;

        // 如果 buffer_ == NULL，则 realloc 相当于 malloc。
        newBuf = realloc(buffer_, size + 1);  // 多分配一个字节用于 c_str()!

        if (newBuf)
        {
            if (initZero && (size > size_))
                memset(((char*)newBuf) + size_, 0, size - size_);
            buffer_ = newBuf;
            size_ = size;
            verifyPosition();
        }
        else
        {
            iseThrowMemoryException();
        }
    }
}

//-----------------------------------------------------------------------------
// 描述: 设置 Position
//-----------------------------------------------------------------------------
void Buffer::setPosition(int position)
{
    position_ = position;
    verifyPosition();
}

//-----------------------------------------------------------------------------
// 描述: 返回 C 风格的字符串 (末端附加结束符 '\0')
//-----------------------------------------------------------------------------
char* Buffer::c_str() const
{
    if (size_ <= 0 || !buffer_)
        return (char*)"";
    else
    {
        ((char*)buffer_)[size_] = 0;
        return (char*)buffer_;
    }
}

//-----------------------------------------------------------------------------
// 描述: 将其它流读入到缓存中
//-----------------------------------------------------------------------------
bool Buffer::loadFromStream(Stream& stream)
{
    try
    {
        INT64 size64 = stream.getSize() - stream.getPosition();
        ISE_ASSERT(size64 <= MAXLONG);
        int size = (int)size64;

        setPosition(0);
        setSize(size);
        stream.readBuffer(data(), size);
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 将文件读入到缓存中
//-----------------------------------------------------------------------------
bool Buffer::loadFromFile(const string& fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
    if (result)
        result = loadFromStream(fs);
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 将缓存保存到其它流中
//-----------------------------------------------------------------------------
bool Buffer::saveToStream(Stream& stream)
{
    try
    {
        if (getSize() > 0)
            stream.writeBuffer(data(), getSize());
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 将缓存保存到文件中
//-----------------------------------------------------------------------------
bool Buffer::saveToFile(const string& fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_CREATE);
    if (result)
        result = saveToStream(fs);
    return result;
}

//-----------------------------------------------------------------------------

void Buffer::assign(const Buffer& src)
{
    setSize(src.getSize());
    setPosition(src.getPosition());
    if (size_ > 0)
        memmove(buffer_, src.buffer_, size_);
}

//-----------------------------------------------------------------------------

void Buffer::verifyPosition()
{
    if (position_ < 0) position_ = 0;
    if (position_ > size_) position_ = size_;
}

///////////////////////////////////////////////////////////////////////////////
// class DateTime

//-----------------------------------------------------------------------------
// 描述: 返回当前时间 (从1970-01-01 00:00:00 算起的秒数, UTC时间)
//-----------------------------------------------------------------------------
DateTime DateTime::currentDateTime()
{
    return DateTime(time(NULL));
}

//-----------------------------------------------------------------------------
// 描述: 将字符串转换成 DateTime
// 注意: dateTimeStr 的格式必须为 YYYY-MM-DD HH:MM:SS
//-----------------------------------------------------------------------------
DateTime& DateTime::operator = (const string& dateTimeStr)
{
    int year, month, day, hour, minute, second;

    if (dateTimeStr.length() == 19)
    {
        year = strToInt(dateTimeStr.substr(0, 4), 0);
        month = strToInt(dateTimeStr.substr(5, 2), 0);
        day = strToInt(dateTimeStr.substr(8, 2), 0);
        hour = strToInt(dateTimeStr.substr(11, 2), 0);
        minute = strToInt(dateTimeStr.substr(14, 2), 0);
        second = strToInt(dateTimeStr.substr(17, 2), 0);

        encodeDateTime(year, month, day, hour, minute, second);
        return *this;
    }
    else
    {
        iseThrowException(SEM_INVALID_DATETIME_STR);
        return *this;
    }
}

//-----------------------------------------------------------------------------
// 描述: 日期时间编码，并存入 *this
//-----------------------------------------------------------------------------
void DateTime::encodeDateTime(int year, int month, int day,
    int hour, int minute, int second)
{
    struct tm tm;

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

    time_ = mktime(&tm);
}

//-----------------------------------------------------------------------------
// 描述: 日期时间解码，并存入各参数
//-----------------------------------------------------------------------------
void DateTime::decodeDateTime(int *year, int *month, int *day,
    int *hour, int *minute, int *second, int *weekDay, int *yearDay) const
{
    struct tm tm;

#ifdef ISE_WINDOWS
#ifdef ISE_COMPILER_VC
#if (_MSC_VER >= 1400)  // >= VC8
    localtime_s(&tm, &time_);
#else
    struct tm *ptm = localtime(&time_);    // 存在重入隐患！
    if (ptm) tm = *ptm;
#endif
#else
    struct tm *ptm = localtime(&time_);    // 存在重入隐患！
    if (ptm) tm = *ptm;
#endif
#endif

#ifdef ISE_LINUX
    localtime_r(&time_, &tm);
#endif

    if (year) *year = tm.tm_year + 1900;
    if (month) *month = tm.tm_mon + 1;
    if (day) *day = tm.tm_mday;
    if (hour) *hour = tm.tm_hour;
    if (minute) *minute = tm.tm_min;
    if (second) *second = tm.tm_sec;
    if (weekDay) *weekDay = tm.tm_wday;
    if (yearDay) *yearDay = tm.tm_yday;
}

//-----------------------------------------------------------------------------
// 描述: 返回日期字符串
// 参数:
//   dateSep - 日期分隔符
// 格式:
//   YYYY-MM-DD
//-----------------------------------------------------------------------------
string DateTime::dateString(const string& dateSep) const
{
    string result;
    int year, month, day;

    decodeDateTime(&year, &month, &day, NULL, NULL, NULL, NULL);
    result = formatString("%04d%s%02d%s%02d",
        year, dateSep.c_str(), month, dateSep.c_str(), day);

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 返回日期时间字符串
// 参数:
//   dateSep     - 日期分隔符
//   dateTimeSep - 日期和时间之间的分隔符
//   timeSep     - 时间分隔符
// 格式:
//   YYYY-MM-DD HH:MM:SS
//-----------------------------------------------------------------------------
string DateTime::dateTimeString(const string& dateSep,
    const string& dateTimeSep, const string& timeSep) const
{
    string result;
    int year, month, day, hour, minute, second;

    decodeDateTime(&year, &month, &day, &hour, &minute, &second, NULL);
    result = formatString("%04d%s%02d%s%02d%s%02d%s%02d%s%02d",
        year, dateSep.c_str(), month, dateSep.c_str(), day,
        dateTimeSep.c_str(),
        hour, timeSep.c_str(), minute, timeSep.c_str(), second);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CriticalSection

CriticalSection::CriticalSection()
{
#ifdef ISE_WINDOWS
    InitializeCriticalSection(&lock_);
#endif
#ifdef ISE_LINUX
    pthread_mutexattr_t attr;

    // 锁属性说明:
    // PTHREAD_MUTEX_TIMED_NP:
    //   普通锁。同一线程内必须成对调用 Lock 和 Unlock。不可连续调用多次 Lock，否则会死锁。
    // PTHREAD_MUTEX_RECURSIVE_NP:
    //   嵌套锁。线程内可以嵌套调用 Lock，第一次生效，之后必须调用相同次数的 Unlock 方可解锁。
    // PTHREAD_MUTEX_ERRORCHECK_NP:
    //   检错锁。如果同一线程嵌套调用 Lock 则产生错误。
    // PTHREAD_MUTEX_ADAPTIVE_NP:
    //   适应锁。
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock_, &attr);
    pthread_mutexattr_destroy(&attr);
#endif
}

//-----------------------------------------------------------------------------

CriticalSection::~CriticalSection()
{
#ifdef ISE_WINDOWS
    DeleteCriticalSection(&lock_);
#endif
#ifdef ISE_LINUX
    // 如果在未解锁的情况下 destroy，此函数会返回错误 EBUSY。
    // 在 linux 下，即使此函数返回错误，也不会有资源泄漏。
    pthread_mutex_destroy(&lock_);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 加锁
//-----------------------------------------------------------------------------
void CriticalSection::lock()
{
#ifdef ISE_WINDOWS
    EnterCriticalSection(&lock_);
#endif
#ifdef ISE_LINUX
    pthread_mutex_lock(&lock_);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 解锁
//-----------------------------------------------------------------------------
void CriticalSection::unlock()
{
#ifdef ISE_WINDOWS
    LeaveCriticalSection(&lock_);
#endif
#ifdef ISE_LINUX
    pthread_mutex_unlock(&lock_);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 尝试加锁 (若已经处于加锁状态则立即返回)
// 返回:
//   true   - 加锁成功
//   false  - 失败，此锁已经处于加锁状态
//-----------------------------------------------------------------------------
bool CriticalSection::tryLock()
{
#ifdef ISE_WINDOWS
    return TryEnterCriticalSection(&lock_) != 0;
#endif
#ifdef ISE_LINUX
    return pthread_mutex_trylock(&lock_) != EBUSY;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class Semaphore

Semaphore::Semaphore(UINT initValue)
{
    initValue_ = initValue;
    doCreateSem();
}

//-----------------------------------------------------------------------------

Semaphore::~Semaphore()
{
    doDestroySem();
}

//-----------------------------------------------------------------------------
// 描述: 将旗标值原子地加1，表示增加一个可访问的资源
//-----------------------------------------------------------------------------
void Semaphore::increase()
{
#ifdef ISE_WINDOWS
    ReleaseSemaphore(sem_, 1, NULL);
#endif
#ifdef ISE_LINUX
    sem_post(&sem_);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 等待旗标(直到旗标值大于0)，然后将旗标值原子地减1
//-----------------------------------------------------------------------------
void Semaphore::wait()
{
#ifdef ISE_WINDOWS
    WaitForSingleObject(sem_, INFINITE);
#endif
#ifdef ISE_LINUX
    int ret;
    do
    {
        ret = sem_wait(&sem_);
    }
    while (ret == -1 && (errno == EINTR || errno == EAGAIN));
#endif
}

//-----------------------------------------------------------------------------
// 描述: 将旗标重置，其计数值设为初始状态
//-----------------------------------------------------------------------------
void Semaphore::reset()
{
    doDestroySem();
    doCreateSem();
}

//-----------------------------------------------------------------------------

void Semaphore::doCreateSem()
{
#ifdef ISE_WINDOWS
    sem_ = CreateSemaphore(NULL, initValue_, 0x7FFFFFFF, NULL);
    if (!sem_)
        iseThrowException(SEM_SEM_INIT_ERROR);
#endif
#ifdef ISE_LINUX
    if (sem_init(&sem_, 0, initValue_) != 0)
        iseThrowException(SEM_SEM_INIT_ERROR);
#endif
}

//-----------------------------------------------------------------------------

void Semaphore::doDestroySem()
{
#ifdef ISE_WINDOWS
    CloseHandle(sem_);
#endif
#ifdef ISE_LINUX
    sem_destroy(&sem_);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class SignalMasker

#ifdef ISE_LINUX

SignalMasker::SignalMasker(bool isAutoRestore) :
    isBlock_(false),
    isAutoRestore_(isAutoRestore)
{
    sigemptyset(&oldSet_);
    sigemptyset(&newSet_);
}

//-----------------------------------------------------------------------------

SignalMasker::~SignalMasker()
{
    if (isAutoRestore_) restore();
}

//-----------------------------------------------------------------------------
// 描述: 设置 Block/UnBlock 操作所需的信号集合
//-----------------------------------------------------------------------------
void SignalMasker::setSignals(int sigCount, va_list argList)
{
    sigemptyset(&newSet_);
    for (int i = 0; i < sigCount; i++)
        sigaddset(&newSet_, va_arg(argList, int));
}

//-----------------------------------------------------------------------------

void SignalMasker::setSignals(int sigCount, ...)
{
    va_list argList;
    va_start(argList, sigCount);
    setSignals(sigCount, argList);
    va_end(argList);
}

//-----------------------------------------------------------------------------
// 描述: 在进程当前阻塞信号集中添加 setSignals 设置的信号
//-----------------------------------------------------------------------------
void SignalMasker::block()
{
    sigProcMask(SIG_BLOCK, &newSet_, &oldSet_);
    isBlock_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 在进程当前阻塞信号集中解除 setSignals 设置的信号
//-----------------------------------------------------------------------------
void SignalMasker::unBlock()
{
    sigProcMask(SIG_UNBLOCK, &newSet_, &oldSet_);
    isBlock_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 将进程阻塞信号集恢复为 Block/UnBlock 之前的状态
//-----------------------------------------------------------------------------
void SignalMasker::restore()
{
    if (isBlock_)
    {
        sigProcMask(SIG_SETMASK, &oldSet_, NULL);
        isBlock_ = false;
    }
}

//-----------------------------------------------------------------------------

int SignalMasker::sigProcMask(int how, const sigset_t *newSet, sigset_t *oldSet)
{
    int result;
    if ((result = sigprocmask(how, newSet, oldSet)) < 0)
        iseThrowException(strerror(errno));

    return result;
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class SeqNumberAlloc

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   startId - 起始序列号
//-----------------------------------------------------------------------------
SeqNumberAlloc::SeqNumberAlloc(UINT64 startId)
{
    currentId_ = startId;
}

//-----------------------------------------------------------------------------
// 描述: 返回一个新分配的ID
//-----------------------------------------------------------------------------
UINT64 SeqNumberAlloc::allocId()
{
    AutoLocker locker(lock_);
    return currentId_++;
}

///////////////////////////////////////////////////////////////////////////////
// class Stream

INT64 Stream::getSize()
{
    INT64 pos, result;

    pos = seek(0, SO_CURRENT);
    result = seek(0, SO_END);
    seek(pos, SO_BEGINNING);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class MemoryStream

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   memoryDelta - 内存增长步长 (字节数，必须是 2 的 N 次方)
//-----------------------------------------------------------------------------
MemoryStream::MemoryStream(int memoryDelta) :
    memory_(NULL),
    capacity_(0),
    size_(0),
    position_(0)
{
    setMemoryDelta(memoryDelta);
}

//-----------------------------------------------------------------------------

MemoryStream::~MemoryStream()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 读内存流
//-----------------------------------------------------------------------------
int MemoryStream::read(void *buffer, int count)
{
    int result = 0;

    if (position_ >= 0 && count >= 0)
    {
        result = size_ - position_;
        if (result > 0)
        {
            if (result > count) result = count;
            memmove(buffer, memory_ + (UINT)position_, result);
            position_ += result;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 写内存流
//-----------------------------------------------------------------------------
int MemoryStream::write(const void *buffer, int count)
{
    int result = 0;
    int pos;

    if (position_ >= 0 && count >= 0)
    {
        pos = position_ + count;
        if (pos > 0)
        {
            if (pos > size_)
            {
                if (pos > capacity_)
                    setCapacity(pos);
                size_ = pos;
            }
            memmove(memory_ + (UINT)position_, buffer, count);
            position_ = pos;
            result = count;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 内存流指针定位
//-----------------------------------------------------------------------------
INT64 MemoryStream::seek(INT64 offset, SEEK_ORIGIN seekOrigin)
{
    switch (seekOrigin)
    {
    case SO_BEGINNING:
        position_ = (int)offset;
        break;
    case SO_CURRENT:
        position_ += (int)offset;
        break;
    case SO_END:
        position_ = size_ + (int)offset;
        break;
    }

    return position_;
}

//-----------------------------------------------------------------------------

void Stream::readBuffer(void *buffer, int count)
{
    if (count != 0 && read(buffer, count) != count)
        iseThrowStreamException(SEM_STREAM_READ_ERROR);
}

//-----------------------------------------------------------------------------

void Stream::writeBuffer(const void *buffer, int count)
{
    if (count != 0 && write(buffer, count) != count)
        iseThrowStreamException(SEM_STREAM_WRITE_ERROR);
}

//-----------------------------------------------------------------------------
// 描述: 设置内存流大小
//-----------------------------------------------------------------------------
void MemoryStream::setSize(INT64 size)
{
    ISE_ASSERT(size <= MAXLONG);

    int oldPos = position_;

    setCapacity((int)size);
    size_ = (int)size;
    if (oldPos > size) seek(0, SO_END);
}

//-----------------------------------------------------------------------------
// 描述: 将其它流读入到内存流中
//-----------------------------------------------------------------------------
bool MemoryStream::loadFromStream(Stream& stream)
{
    try
    {
        INT64 count = stream.getSize();
        ISE_ASSERT(count <= MAXLONG);

        stream.setPosition(0);
        setSize(count);
        if (count != 0)
            stream.readBuffer(memory_, (int)count);
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 将文件读入到内存流中
//-----------------------------------------------------------------------------
bool MemoryStream::loadFromFile(const string& fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
    if (result)
        result = loadFromStream(fs);
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 将内存流保存到其它流中
//-----------------------------------------------------------------------------
bool MemoryStream::saveToStream(Stream& stream)
{
    try
    {
        if (size_ != 0)
            stream.writeBuffer(memory_, size_);
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 将内存流保存到文件中
//-----------------------------------------------------------------------------
bool MemoryStream::saveToFile(const string& fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_CREATE);
    if (result)
        result = saveToStream(fs);
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 清空内存流
//-----------------------------------------------------------------------------
void MemoryStream::clear()
{
    setCapacity(0);
    size_ = 0;
    position_ = 0;
}

//-----------------------------------------------------------------------------

void MemoryStream::setMemoryDelta(int newMemoryDelta)
{
    if (newMemoryDelta != DEFAULT_MEMORY_DELTA)
    {
        if (newMemoryDelta < MIN_MEMORY_DELTA)
            newMemoryDelta = MIN_MEMORY_DELTA;

        // 保证 newMemoryDelta 是2的N次方
        for (int i = sizeof(int) * 8 - 1; i >= 0; i--)
            if (((1 << i) & newMemoryDelta) != 0)
            {
                newMemoryDelta &= (1 << i);
                break;
            }
    }

    memoryDelta_ = newMemoryDelta;
}

//-----------------------------------------------------------------------------

void MemoryStream::setPointer(char *memory, int size)
{
    memory_ = memory;
    size_ = size;
}

//-----------------------------------------------------------------------------

void MemoryStream::setCapacity(int newCapacity)
{
    setPointer(realloc(newCapacity), size_);
    capacity_ = newCapacity;
}

//-----------------------------------------------------------------------------

char* MemoryStream::realloc(int& newCapacity)
{
    char* result;

    if (newCapacity > 0 && newCapacity != size_)
        newCapacity = (newCapacity + (memoryDelta_ - 1)) & ~(memoryDelta_ - 1);

    result = memory_;
    if (newCapacity != capacity_)
    {
        if (newCapacity == 0)
        {
            free(memory_);
            result = NULL;
        }
        else
        {
            if (capacity_ == 0)
                result = (char*)malloc(newCapacity);
            else
                result = (char*)::realloc(memory_, newCapacity);

            if (!result)
                iseThrowMemoryException();
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class FileStream

//-----------------------------------------------------------------------------
// 描述: 缺省构造函数
//-----------------------------------------------------------------------------
FileStream::FileStream()
{
    init();
}

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   fileName   - 文件名
//   openMode   - 文件流打开方式
//   rights     - 文件存取权限
//-----------------------------------------------------------------------------
FileStream::FileStream(const string& fileName, UINT openMode, UINT rights)
{
    init();

    FileException e;
    if (!open(fileName, openMode, rights, &e))
        throw FileException(e);
}

//-----------------------------------------------------------------------------
// 描述: 析构函数
//-----------------------------------------------------------------------------
FileStream::~FileStream()
{
    close();
}

//-----------------------------------------------------------------------------
// 描述: 打开文件
// 参数:
//   fileName - 文件名
//   openMode   - 文件流打开方式
//   rights     - 文件存取权限
//   exception  - 如果发生异常，则传回该异常
//-----------------------------------------------------------------------------
bool FileStream::open(const string& fileName, UINT openMode, UINT rights,
    FileException* exception)
{
    close();

    if (openMode == FM_CREATE)
        handle_ = fileCreate(fileName, rights);
    else
        handle_ = fileOpen(fileName, openMode);

    bool result = (handle_ != INVALID_HANDLE_VALUE);

    if (!result && exception != NULL)
    {
        if (openMode == FM_CREATE)
            *exception = FileException(fileName.c_str(), getLastSysError(),
                formatString(SEM_CANNOT_CREATE_FILE, fileName.c_str(),
                sysErrorMessage(getLastSysError()).c_str()).c_str());
        else
            *exception = FileException(fileName.c_str(), getLastSysError(),
                formatString(SEM_CANNOT_OPEN_FILE, fileName.c_str(),
                sysErrorMessage(getLastSysError()).c_str()).c_str());
    }

    fileName_ = fileName;
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 关闭文件
//-----------------------------------------------------------------------------
void FileStream::close()
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        fileClose(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
    fileName_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 读文件流
//-----------------------------------------------------------------------------
int FileStream::read(void *buffer, int count)
{
    int result;

    result = fileRead(handle_, buffer, count);
    if (result == -1) result = 0;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 写文件流
//-----------------------------------------------------------------------------
int FileStream::write(const void *buffer, int count)
{
    int result;

    result = fileWrite(handle_, buffer, count);
    if (result == -1) result = 0;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 文件流指针定位
//-----------------------------------------------------------------------------
INT64 FileStream::seek(INT64 offset, SEEK_ORIGIN seekOrigin)
{
    return fileSeek(handle_, offset, seekOrigin);
}

//-----------------------------------------------------------------------------
// 描述: 设置文件流大小
//-----------------------------------------------------------------------------
void FileStream::setSize(INT64 size)
{
    bool success;
    seek(size, SO_BEGINNING);

#ifdef ISE_WINDOWS
    success = ::SetEndOfFile(handle_) != 0;
#endif
#ifdef ISE_LINUX
    success = (::ftruncate(handle_, getPosition()) == 0);
#endif

    if (!success)
        iseThrowFileException(fileName_.c_str(), getLastSysError(), SEM_SET_FILE_STREAM_SIZE_ERR);
}

//-----------------------------------------------------------------------------
// 描述: 判断文件流当前是否打开状态
//-----------------------------------------------------------------------------
bool FileStream::isOpen() const
{
    return (handle_ != INVALID_HANDLE_VALUE);
}

//-----------------------------------------------------------------------------
// 描述: 初始化
//-----------------------------------------------------------------------------
void FileStream::init()
{
    fileName_.clear();
    handle_ = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// 描述: 创建文件
//-----------------------------------------------------------------------------
HANDLE FileStream::fileCreate(const string& fileName, UINT rights)
{
#ifdef ISE_WINDOWS
    return ::CreateFileA(fileName.c_str(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#endif
#ifdef ISE_LINUX
    umask(0);  // 防止 rights 被 umask 值 遮掩
    return ::open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, rights);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 打开文件
//-----------------------------------------------------------------------------
HANDLE FileStream::fileOpen(const string& fileName, UINT openMode)
{
#ifdef ISE_WINDOWS
    UINT accessModes[3] = {
        GENERIC_READ,
        GENERIC_WRITE,
        GENERIC_READ | GENERIC_WRITE
    };
    UINT shareModes[5] = {
        0,
        0,
        FILE_SHARE_READ,
        FILE_SHARE_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE
    };

    HANDLE fileHandle = INVALID_HANDLE_VALUE;

    if ((openMode & 3) <= FM_OPEN_READ_WRITE &&
        (openMode & 0xF0) <= FM_SHARE_DENY_NONE)
        fileHandle = ::CreateFileA(fileName.c_str(), accessModes[openMode & 3],
            shareModes[(openMode & 0xF0) >> 4], NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    return fileHandle;
#endif
#ifdef ISE_LINUX
    BYTE shareModes[4] = {
        0,          // none
        F_WRLCK,    // FM_SHARE_EXCLUSIVE
        F_RDLCK,    // FM_SHARE_DENY_WRITE
        0           // FM_SHARE_DENY_NONE
    };

    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    BYTE shareMode;

    if (fileExists(fileName) &&
        (openMode & 0x03) <= FM_OPEN_READ_WRITE &&
        (openMode & 0xF0) <= FM_SHARE_DENY_NONE)
    {
        umask(0);  // 防止 openMode 被 umask 值遮掩
        fileHandle = ::open(fileName.c_str(), (openMode & 0x03), DEFAULT_FILE_ACCESS_RIGHTS);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            shareMode = ((openMode & 0xF0) >> 4);
            if (shareModes[shareMode] != 0)
            {
                struct flock flk;

                flk.l_type = shareModes[shareMode];
                flk.l_whence = SEEK_SET;
                flk.l_start = 0;
                flk.l_len = 0;

                if (fcntl(fileHandle, F_SETLK, &flk) < 0)
                {
                    fileClose(fileHandle);
                    fileHandle = INVALID_HANDLE_VALUE;
                }
            }
        }
    }

    return fileHandle;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 关闭文件
//-----------------------------------------------------------------------------
void FileStream::fileClose(HANDLE handle)
{
#ifdef ISE_WINDOWS
    ::CloseHandle(handle);
#endif
#ifdef ISE_LINUX
    ::close(handle);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 读文件
//-----------------------------------------------------------------------------
int FileStream::fileRead(HANDLE handle, void *buffer, int count)
{
#ifdef ISE_WINDOWS
    unsigned long result;
    if (!::ReadFile(handle, buffer, count, &result, NULL))
        result = -1;
    return result;
#endif
#ifdef ISE_LINUX
    return ::read(handle, buffer, count);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 写文件
//-----------------------------------------------------------------------------
int FileStream::fileWrite(HANDLE handle, const void *buffer, int count)
{
#ifdef ISE_WINDOWS
    unsigned long result;
    if (!::WriteFile(handle, buffer, count, &result, NULL))
        result = -1;
    return result;
#endif
#ifdef ISE_LINUX
    return ::write(handle, buffer, count);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 文件指针定位
//-----------------------------------------------------------------------------
INT64 FileStream::fileSeek(HANDLE handle, INT64 offset, SEEK_ORIGIN seekOrigin)
{
#ifdef ISE_WINDOWS
    INT64 result = offset;
    ((Int64Rec*)&result)->ints.lo = ::SetFilePointer(
        handle, ((Int64Rec*)&result)->ints.lo,
        (PLONG)&(((Int64Rec*)&result)->ints.hi), seekOrigin);
    if (((Int64Rec*)&result)->ints.lo == -1 && GetLastError() != 0)
        ((Int64Rec*)&result)->ints.hi = -1;
    return result;
#endif
#ifdef ISE_LINUX
    return ::lseek(handle, offset, seekOrigin);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class PointerList

PointerList::PointerList() :
    list_(NULL),
    count_(0),
    capacity_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------

PointerList::~PointerList()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 向列表中添加元素
//-----------------------------------------------------------------------------
int PointerList::add(POINTER item)
{
    if (count_ == capacity_) grow();
    list_[count_] = item;
    count_++;

    return count_ - 1;
}

//-----------------------------------------------------------------------------
// 描述: 向列表中插入元素
// 参数:
//   index - 插入位置下标号(0-based)
//-----------------------------------------------------------------------------
void PointerList::insert(int index, POINTER item)
{
    if (index < 0 || index > count_)
        iseThrowException(SEM_INDEX_ERROR);

    if (count_ == capacity_) grow();
    if (index < count_)
        memmove(&list_[index + 1], &list_[index], (count_ - index) * sizeof(POINTER));
    list_[index] = item;
    count_++;
}

//-----------------------------------------------------------------------------
// 描述: 从列表中删除元素
// 参数:
//   index - 下标号(0-based)
//-----------------------------------------------------------------------------
void PointerList::del(int index)
{
    if (index < 0 || index >= count_)
        iseThrowException(SEM_INDEX_ERROR);

    count_--;
    if (index < count_)
        memmove(&list_[index], &list_[index + 1], (count_ - index) * sizeof(POINTER));
}

//-----------------------------------------------------------------------------
// 描述: 从列表中删除元素
// 返回: 被删除元素在列表中的下标号(0-based)，若未找到，则返回 -1.
//-----------------------------------------------------------------------------
int PointerList::remove(POINTER item)
{
    int result;

    result = indexOf(item);
    if (result >= 0)
        del(result);

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 从列表中删除元素
// 返回: 被删除的元素值，若未找到，则返回 NULL.
//-----------------------------------------------------------------------------
POINTER PointerList::extract(POINTER item)
{
    int i;
    POINTER result = NULL;

    i = indexOf(item);
    if (i >= 0)
    {
        result = item;
        list_[i] = NULL;
        del(i);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 移动一个元素到新的位置
//-----------------------------------------------------------------------------
void PointerList::move(int curIndex, int newIndex)
{
    POINTER item;

    if (curIndex != newIndex)
    {
        if (newIndex < 0 || newIndex >= count_)
            iseThrowException(SEM_INDEX_ERROR);

        item = get(curIndex);
        list_[curIndex] = NULL;
        del(curIndex);
        insert(newIndex, NULL);
        list_[newIndex] = item;
    }
}

//-----------------------------------------------------------------------------
// 描述: 改变列表的大小
//-----------------------------------------------------------------------------
void PointerList::resize(int count)
{
    setCount(count);
}

//-----------------------------------------------------------------------------
// 描述: 清空列表
//-----------------------------------------------------------------------------
void PointerList::clear()
{
    setCount(0);
    setCapacity(0);
}

//-----------------------------------------------------------------------------
// 描述: 返回列表中的首个元素 (若列表为空则抛出异常)
//-----------------------------------------------------------------------------
POINTER PointerList::first() const
{
    return get(0);
}

//-----------------------------------------------------------------------------
// 描述: 返回列表中的最后元素 (若列表为空则抛出异常)
//-----------------------------------------------------------------------------
POINTER PointerList::last() const
{
    return get(count_ - 1);
}

//-----------------------------------------------------------------------------
// 描述: 返回元素在列表中的下标号 (若未找到则返回 -1)
//-----------------------------------------------------------------------------
int PointerList::indexOf(POINTER item) const
{
    int result = 0;

    while (result < count_ && list_[result] != item) result++;
    if (result == count_)
        result = -1;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 返回列表中元素总数
//-----------------------------------------------------------------------------
int PointerList::getCount() const
{
    return count_;
}

//-----------------------------------------------------------------------------
// 描述: 赋值操作符
//-----------------------------------------------------------------------------
PointerList& PointerList::operator = (const PointerList& rhs)
{
    if (this == &rhs) return *this;

    clear();
    setCapacity(rhs.capacity_);
    for (int i = 0; i < rhs.getCount(); i++)
        add(rhs[i]);
    return *this;
}

//-----------------------------------------------------------------------------
// 描述: 存取列表中任意元素
//-----------------------------------------------------------------------------
const POINTER& PointerList::operator[] (int index) const
{
    if (index < 0 || index >= count_)
        iseThrowException(SEM_INDEX_ERROR);

    return list_[index];
}

//-----------------------------------------------------------------------------
// 描述: 存取列表中任意元素
//-----------------------------------------------------------------------------
POINTER& PointerList::operator[] (int index)
{
    return
        const_cast<POINTER&>(
            ((const PointerList&)(*this))[index]
        );
}

//-----------------------------------------------------------------------------

void PointerList::grow()
{
    int delta;

    if (capacity_ > 64)
        delta = capacity_ / 4;
    else if (capacity_ > 8)
        delta = 16;
    else
        delta = 4;

    setCapacity(capacity_ + delta);
}

//-----------------------------------------------------------------------------

POINTER PointerList::get(int index) const
{
    if (index < 0 || index >= count_)
        iseThrowException(SEM_INDEX_ERROR);

    return list_[index];
}

//-----------------------------------------------------------------------------

void PointerList::put(int index, POINTER item)
{
    if (index < 0 || index >= count_)
        iseThrowException(SEM_INDEX_ERROR);

    list_[index] = item;
}

//-----------------------------------------------------------------------------

void PointerList::setCapacity(int newCapacity)
{
    if (newCapacity < count_)
        iseThrowException(SEM_LIST_CAPACITY_ERROR);

    if (newCapacity != capacity_)
    {
        if (newCapacity > 0)
        {
            POINTER *p = (POINTER*)realloc(list_, newCapacity * sizeof(PVOID));
            if (p)
                list_ = p;
            else
                iseThrowMemoryException();
        }
        else
        {
            if (list_)
            {
                free(list_);
                list_ = NULL;
            }
        }

        capacity_ = newCapacity;
    }
}

//-----------------------------------------------------------------------------

void PointerList::setCount(int newCount)
{
    if (newCount < 0)
        iseThrowException(SEM_LIST_COUNT_ERROR);

    if (newCount > capacity_)
        setCapacity(newCount);
    if (newCount > count_)
        memset(&list_[count_], 0, (newCount - count_) * sizeof(POINTER));
    else
        for (int i = count_ - 1; i >= newCount; i--) del(i);

    count_ = newCount;
}

///////////////////////////////////////////////////////////////////////////////
// class PropertyList

PropertyList::PropertyList()
{
    // nothing
}

//-----------------------------------------------------------------------------

PropertyList::~PropertyList()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 向列表中添加元素
//-----------------------------------------------------------------------------
void PropertyList::add(const string& name, const string& value)
{
    if (name.find(NAME_VALUE_SEP, 0) != string::npos)
        iseThrowException(SEM_PROPLIST_NAME_ERROR);

    PropertyItem *item = find(name);
    if (!item)
    {
        item = new PropertyItem(name, value);
        items_.add(item);
    }
    else
        item->value = value;
}

//-----------------------------------------------------------------------------
// 描述: 从列表中删除元素
// 返回:
//   true  - 成功
//   false - 失败(未找到)
//-----------------------------------------------------------------------------
bool PropertyList::remove(const string& name)
{
    int i = indexOf(name);
    bool result = (i >= 0);

    if (result)
    {
        delete (PropertyItem*)items_[i];
        items_.del(i);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 清空列表
//-----------------------------------------------------------------------------
void PropertyList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (PropertyItem*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 返回某项属性在列表中的位置(0-based)
//-----------------------------------------------------------------------------
int PropertyList::indexOf(const string& name) const
{
    int result = -1;

    for (int i = 0; i < items_.getCount(); i++)
        if (sameText(name, ((PropertyItem*)items_[i])->name))
        {
            result = i;
            break;
        }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断某项属性是否存在
//-----------------------------------------------------------------------------
bool PropertyList::nameExists(const string& name) const
{
    return (indexOf(name) >= 0);
}

//-----------------------------------------------------------------------------
// 描述: 根据 Name 从列表中取出 Value
// 返回: 若列表中不存在该项属性，则返回 False。
//-----------------------------------------------------------------------------
bool PropertyList::getValue(const string& name, string& value) const
{
    int i = indexOf(name);
    bool result = (i >= 0);

    if (result)
        value = ((PropertyItem*)items_[i])->value;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 根据下标号(0-based)取得属性项目
//-----------------------------------------------------------------------------
const PropertyList::PropertyItem& PropertyList::getItems(int index) const
{
    if (index < 0 || index >= getCount())
        iseThrowException(SEM_INDEX_ERROR);

    return *((PropertyItem*)items_[index]);
}

//-----------------------------------------------------------------------------
// 描述: 将整个列表转换成 PropString 并返回
// 示例:
//   [<abc,123>, <def,456>]   ->  abc=123,def=456
//   [<abc,123>, <,456>]      ->  abc=123,=456
//   [<abc,123>, <",456>]     ->  abc=123,"""=456"
//   [<abc,123>, <',456>]     ->  abc=123,'=456
//   [<abc,123>, <def,">]     ->  abc=123,"def="""
//   [<abc,123>, < def,456>]  ->  abc=123," def=456"
//   [<abc,123>, <def,=>]     ->  abc=123,def==
//   [<abc,123>, <=,456>]     ->  抛出异常(Name中不允许存在等号"=")
//-----------------------------------------------------------------------------
string PropertyList::getPropString() const
{
    string result;
    string itemStr;

    for (int index = 0; index < getCount(); index++)
    {
        const PropertyItem& item = getItems(index);

        itemStr = item.name + (char)NAME_VALUE_SEP + item.value;
        if (hasReservedChar(itemStr))
            itemStr = makeQuotedStr(itemStr);

        result += itemStr;
        if (index < getCount() - 1) result += (char)PROP_ITEM_SEP;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 将 PropString 转换成属性列表
//-----------------------------------------------------------------------------
void PropertyList::setPropString(const string& propString)
{
    char *strPtr = const_cast<char*>(propString.c_str());
    string itemStr;

    clear();
    while (*strPtr)
    {
        if (*strPtr == PROP_ITEM_QUOT)
            itemStr = extractQuotedStr(strPtr);
        else
        {
            char *p = strPtr;
            strPtr = scanStr(strPtr, PROP_ITEM_SEP);
            itemStr.assign(p, strPtr - p);
            if (*strPtr == PROP_ITEM_SEP) strPtr++;
        }

        string::size_type i = itemStr.find(NAME_VALUE_SEP, 0);
        if (i != string::npos)
            add(itemStr.substr(0, i), itemStr.substr(i + 1));
    }
}

//-----------------------------------------------------------------------------
// 描述: 赋值操作符
//-----------------------------------------------------------------------------
PropertyList& PropertyList::operator = (const PropertyList& rhs)
{
    if (this == &rhs) return *this;

    clear();
    for (int i = 0; i < rhs.getCount(); i++)
        add(rhs.getItems(i).name, rhs.getItems(i).value);

    return *this;
}

//-----------------------------------------------------------------------------
// 描述: 存取列表中任意元素
// 注意: 调用此函数时，若 name 不存在，则立即添加到列表中！
//-----------------------------------------------------------------------------
string& PropertyList::operator[] (const string& name)
{
    int i = indexOf(name);
    if (i < 0)
    {
        add(name, "");
        i = getCount() - 1;
    }

    return ((PropertyItem*)items_[i])->value;
}

//-----------------------------------------------------------------------------
// 描述: 查找某个属性项目，没找到则返回 NULL
//-----------------------------------------------------------------------------
PropertyList::PropertyItem* PropertyList::find(const string& name)
{
    int i = indexOf(name);
    PropertyItem *result = (i >= 0? (PropertyItem*)items_[i] : NULL);
    return result;
}

//-----------------------------------------------------------------------------

bool PropertyList::isReservedChar(char ch)
{
    return
        ((ch >= 0) && (ch <= 32)) ||
        (ch == (char)PROP_ITEM_SEP) ||
        (ch == (char)PROP_ITEM_QUOT);
}

//-----------------------------------------------------------------------------

bool PropertyList::hasReservedChar(const string& str)
{
    for (UINT i = 0; i < str.length(); i++)
        if (isReservedChar(str[i])) return true;
    return false;
}

//-----------------------------------------------------------------------------

char* PropertyList::scanStr(char *str, char ch)
{
    char *result = str;
    while ((*result != ch) && (*result != 0)) result++;
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 将字符串 str 用引号(")括起来
// 示例:
//   abcdefg   ->  "abcdefg"
//   abc"efg   ->  "abc""efg"
//   abc""fg   ->  "abc""""fg"
//-----------------------------------------------------------------------------
string PropertyList::makeQuotedStr(const string& str)
{
    const char QUOT_CHAR = (char)PROP_ITEM_QUOT;
    string result;

    for (UINT i = 0; i < str.length(); i++)
    {
        if (str[i] == QUOT_CHAR)
            result += string(2, QUOT_CHAR);
        else
            result += str[i];
    }
    result = "\"" + result + "\"";

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 将 MakeQuotedStr 生成的字符串还原
// 参数:
//   strPtr - 待处理的字符串指针，处理完后传回处理截止位置
// 返回:
//   还原后的字符串
// 示例:
//   "abcde"xyz   ->  abcde       函数返回时 strPtr 指向 x
//   "ab""cd"     ->  ab"cd       函数返回时 strPtr 指向 '\0'
//   "ab""""cd"   ->  ab""cd      函数返回时 strPtr 指向 '\0'
//-----------------------------------------------------------------------------
string PropertyList::extractQuotedStr(char*& strPtr)
{
    const char QUOT_CHAR = (char)PROP_ITEM_QUOT;
    string result;
    int dropCount;
    char *p;

    if (strPtr == NULL || *strPtr != QUOT_CHAR) return result;

    strPtr++;
    dropCount = 1;
    p = strPtr;
    strPtr = scanStr(strPtr, QUOT_CHAR);
    while (*strPtr)
    {
        strPtr++;
        if (*strPtr != QUOT_CHAR) break;
        strPtr++;
        dropCount++;
        strPtr = scanStr(strPtr, QUOT_CHAR);
    }

    if (((strPtr - p) <= 1) || ((strPtr - p - dropCount) == 0)) return result;

    if (dropCount == 1)
        result.assign(p, strPtr - p - 1);
    else
    {
        result.resize(strPtr - p - dropCount);
        char *dest = const_cast<char*>(result.c_str());
        while (p < strPtr)
        {
            if ((*p == QUOT_CHAR) && (p < strPtr - 1) && (*(p+1) == QUOT_CHAR)) p++;
            *dest++ = *p++;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class Strings

Strings::Strings()
{
    init();
}

//-----------------------------------------------------------------------------

int Strings::add(const char* str)
{
    int result = getCount();
    insert(result, str);
    return result;
}

//-----------------------------------------------------------------------------

int Strings::add(const char* str, POINTER data)
{
    int result = add(str);
    setData(result, data);
    return result;
}

//-----------------------------------------------------------------------------

void Strings::addStrings(const Strings& strings)
{
    AutoUpdater autoUpdater(*this);

    for (int i = 0; i < strings.getCount(); i++)
        add(strings.getString(i).c_str(), strings.getData(i));
}

//-----------------------------------------------------------------------------

void Strings::insert(int index, const char* str, POINTER data)
{
    insert(index, str);
    setData(index, data);
}

//-----------------------------------------------------------------------------

bool Strings::equals(const Strings& strings)
{
    bool result;
    int count = getCount();

    result = (count == strings.getCount());
    if (result)
    {
        for (int i = 0; i < count; i++)
            if (getString(i) != strings.getString(i))
            {
                result = false;
                break;
            }
    }

    return result;
}

//-----------------------------------------------------------------------------

void Strings::exchange(int index1, int index2)
{
    AutoUpdater autoUpdater(*this);

    POINTER tempData;
    string tempStr;

    tempStr = getString(index1);
    tempData = getData(index1);
    setString(index1, getString(index2).c_str());
    setData(index1, getData(index2));
    setString(index2, tempStr.c_str());
    setData(index2, tempData);
}

//-----------------------------------------------------------------------------

void Strings::move(int curIndex, int newIndex)
{
    if (curIndex != newIndex)
    {
        AutoUpdater autoUpdater(*this);

        POINTER tempData;
        string tempStr;

        tempStr = getString(curIndex);
        tempData = getData(curIndex);
        del(curIndex);
        insert(newIndex, tempStr.c_str(), tempData);
    }
}

//-----------------------------------------------------------------------------

bool Strings::exists(const char* str) const
{
    return (indexOf(str) >= 0);
}

//-----------------------------------------------------------------------------

int Strings::indexOf(const char* str) const
{
    int result = -1;

    for (int i = 0; i < getCount(); i++)
        if (compareStrings(getString(i).c_str(), str) == 0)
        {
            result = i;
            break;
        }

    return result;
}

//-----------------------------------------------------------------------------

int Strings::indexOfName(const char* name) const
{
    int result = -1;

    for (int i = 0; i < getCount(); i++)
        if (compareStrings((extractName(getString(i).c_str()).c_str()), name) == 0)
        {
            result = i;
            break;
        }

    return result;
}

//-----------------------------------------------------------------------------

int Strings::indexOfData(POINTER data) const
{
    int result = -1;

    for (int i = 0; i < getCount(); i++)
        if (getData(i) == data)
        {
            result = i;
            break;
        }

    return result;
}

//-----------------------------------------------------------------------------

bool Strings::loadFromStream(Stream& stream)
{
    try
    {
        AutoUpdater autoUpdater(*this);

        INT64 size64 = stream.getSize() - stream.getPosition();
        ISE_ASSERT(size64 <= MAXLONG);
        int size = (int)size64;

        Buffer buffer(size + sizeof(char));
        stream.readBuffer(buffer, size);
        *((char*)(buffer.data() + size)) = '\0';

        setText(buffer);
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------

bool Strings::loadFromFile(const char* fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
    if (result)
        result = loadFromStream(fs);
    return result;
}

//-----------------------------------------------------------------------------

bool Strings::saveToStream(Stream& stream) const
{
    try
    {
        string text = getText();
        int len = (int)text.length();
        stream.writeBuffer((char*)text.c_str(), len * sizeof(char));
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------

bool Strings::saveToFile(const char* fileName) const
{
    FileStream fs;
    bool result = fs.open(fileName, FM_CREATE);
    if (result)
        result = saveToStream(fs);
    return result;
}

//-----------------------------------------------------------------------------

char Strings::getDelimiter() const
{
    if ((defined_ & SD_DELIMITER) == 0)
        const_cast<Strings&>(*this).setDelimiter(DEFAULT_DELIMITER);
    return delimiter_;
}

//-----------------------------------------------------------------------------

void Strings::setDelimiter(char value)
{
    if (delimiter_ != value || (defined_ & SD_DELIMITER) == 0)
    {
        defined_ |= SD_DELIMITER;
        delimiter_ = value;
    }
}

//-----------------------------------------------------------------------------

string Strings::getLineBreak() const
{
    if ((defined_ & SD_LINE_BREAK) == 0)
        const_cast<Strings&>(*this).setLineBreak(DEFAULT_LINE_BREAK);
    return lineBreak_;
}

//-----------------------------------------------------------------------------

void Strings::setLineBreak(const char* value)
{
    if (lineBreak_ != string(value) || (defined_ & SD_LINE_BREAK) == 0)
    {
        defined_ |= SD_LINE_BREAK;
        lineBreak_ = value;
    }
}

//-----------------------------------------------------------------------------

char Strings::getQuoteChar() const
{
    if ((defined_ & SD_QUOTE_CHAR) == 0)
        const_cast<Strings&>(*this).setQuoteChar(DEFAULT_QUOTE_CHAR);
    return quoteChar_;
}

//-----------------------------------------------------------------------------

void Strings::setQuoteChar(char value)
{
    if (quoteChar_ != value || (defined_ & SD_QUOTE_CHAR) == 0)
    {
        defined_ |= SD_QUOTE_CHAR;
        quoteChar_ = value;
    }
}

//-----------------------------------------------------------------------------

char Strings::getNameValueSeparator() const
{
    if ((defined_ & SD_NAME_VALUE_SEP) == 0)
        const_cast<Strings&>(*this).setNameValueSeparator(DEFAULT_NAME_VALUE_SEP);
    return nameValueSeparator_;
}

//-----------------------------------------------------------------------------

void Strings::setNameValueSeparator(char value)
{
    if (nameValueSeparator_ != value || (defined_ & SD_NAME_VALUE_SEP) == 0)
    {
        defined_ |= SD_NAME_VALUE_SEP;
        nameValueSeparator_ = value;
    }
}

//-----------------------------------------------------------------------------

string Strings::combineNameValue(const char* name, const char* value) const
{
    string nameStr(name);
    char nameValueSep = getNameValueSeparator();

    if (nameStr.find(nameValueSep) != string::npos)
        error(SEM_STRINGS_NAME_ERROR, 0);

    return nameStr + nameValueSep + value;
}

//-----------------------------------------------------------------------------

string Strings::getName(int index) const
{
    return extractName(getString(index).c_str());
}

//-----------------------------------------------------------------------------

string Strings::getValue(const char* name) const
{
    string nameStr(name);
    int i = indexOfName(nameStr.c_str());
    if (i >= 0)
        return getString(i).substr(nameStr.length() + 1);
    else
        return "";
}

//-----------------------------------------------------------------------------

string Strings::getValue(int index) const
{
    if (index >= 0)
    {
        string name = getName(index);
        if (!name.empty())
            return getString(index).substr(name.length() + 1);
        else
        {
            string strItem = getString(index);
            if (!strItem.empty() && strItem[0] == getNameValueSeparator())
                return strItem.substr(1);
            else
                return "";
        }
    }
    else
        return "";
}

//-----------------------------------------------------------------------------

void Strings::setValue(const char* name, const char* value)
{
    string nameStr(name);
    string valueStr(value);

    int i = indexOfName(nameStr.c_str());
    if (valueStr.empty())
    {
        if (i >= 0) del(i);
    }
    else
    {
        if (i < 0)
            i = add("");
        setString(i, (nameStr + getNameValueSeparator() + valueStr).c_str());
    }
}

//-----------------------------------------------------------------------------

void Strings::setValue(int index, const char* value)
{
    string valueStr(value);

    if (valueStr.empty())
    {
        if (index >= 0) del(index);
    }
    else
    {
        if (index < 0)
            index = add("");
        setString(index, (getName(index) + getNameValueSeparator() + valueStr).c_str());
    }
}

//-----------------------------------------------------------------------------

string Strings::getText() const
{
    string result;
    int count = getCount();
    string lineBreak = getLineBreak();

    for (int i = 0; i < count; i++)
    {
        result += getString(i);
        result += lineBreak;
    }

    return result;
}

//-----------------------------------------------------------------------------

void Strings::setText(const char* value)
{
    AutoUpdater autoUpdater(*this);

    string valueStr(value);
    string lineBreak = getLineBreak();
    int start = 0;

    clear();
    while (true)
    {
        string::size_type pos = valueStr.find(lineBreak, start);
        if (pos != string::npos)
        {
            add(valueStr.substr(start, pos - start).c_str());
            start = (int)(pos + lineBreak.length());
        }
        else
        {
            string str = valueStr.substr(start);
            if (!str.empty())
                add(str.c_str());
            break;
        }
    }
}

//-----------------------------------------------------------------------------

string Strings::getCommaText() const
{
    UINT bakDefined = defined_;
    char bakDelimiter = delimiter_;
    char bakQuoteChar = quoteChar_;

    const_cast<Strings&>(*this).setDelimiter(DEFAULT_DELIMITER);
    const_cast<Strings&>(*this).setQuoteChar(DEFAULT_QUOTE_CHAR);

    string result = getDelimitedText();  // 不可以抛出异常

    const_cast<Strings&>(*this).defined_ = bakDefined;
    const_cast<Strings&>(*this).delimiter_ = bakDelimiter;
    const_cast<Strings&>(*this).quoteChar_ = bakQuoteChar;

    return result;
}

//-----------------------------------------------------------------------------

void Strings::setCommaText(const char* value)
{
    setDelimiter(DEFAULT_DELIMITER);
    setQuoteChar(DEFAULT_QUOTE_CHAR);

    setDelimitedText(value);
}

//-----------------------------------------------------------------------------

string Strings::getDelimitedText() const
{
    string result;
    string line;
    int count = getCount();
    char quoteChar = getQuoteChar();
    char delimiter = getDelimiter();

    if (count == 1 && getString(0).empty())
        return string(getQuoteChar(), 2);

    string delimiters;
    for (int i = 1; i <= 32; i++)
        delimiters += (char)i;
    delimiters += quoteChar;
    delimiters += delimiter;

    for (int i = 0; i < count; i++)
    {
        line = getString(i);
        if (line.find_first_of(delimiters) != string::npos)
            line = getQuotedStr((char*)line.c_str(), quoteChar);

        if (i > 0) result += delimiter;
        result += line;
    }

    return result;
}

//-----------------------------------------------------------------------------

void Strings::setDelimitedText(const char* value)
{
    AutoUpdater autoUpdater(*this);

    char quoteChar = getQuoteChar();
    char delimiter = getDelimiter();
    const char* curPtr = value;
    string line;

    clear();
    while (*curPtr >= 1 && *curPtr <= 32)
        curPtr++;

    while (*curPtr != '\0')
    {
        if (*curPtr == quoteChar)
            line = extractQuotedStr(curPtr, quoteChar);
        else
        {
            const char* p = curPtr;
            while (*curPtr > 32 && *curPtr != delimiter)
                curPtr++;
            line.assign(p, curPtr - p);
        }

        add(line.c_str());

        while (*curPtr >= 1 && *curPtr <= 32)
            curPtr++;

        if (*curPtr == delimiter)
        {
            const char* p = curPtr;
            p++;
            if (*p == '\0')
                add("");

            do curPtr++; while (*curPtr >= 1 && *curPtr <= 32);
        }
    }
}

//-----------------------------------------------------------------------------

void Strings::setString(int index, const char* value)
{
    POINTER tempData = getData(index);
    del(index);
    insert(index, value, tempData);
}

//-----------------------------------------------------------------------------

void Strings::beginUpdate()
{
    if (updateCount_ == 0)
        setUpdateState(true);
    updateCount_++;
}

//-----------------------------------------------------------------------------

void Strings::endUpdate()
{
    updateCount_--;
    if (updateCount_ == 0)
        setUpdateState(false);
}

//-----------------------------------------------------------------------------

Strings& Strings::operator = (const Strings& rhs)
{
    if (this != &rhs)
        assign(rhs);
    return *this;
}

//-----------------------------------------------------------------------------

int Strings::compareStrings(const char* str1, const char* str2) const
{
    int r = compareText(str1, str2);

    if (r > 0) r = 1;
    else if (r < 0) r = -1;

    return r;
}

//-----------------------------------------------------------------------------

void Strings::init()
{
    defined_ = 0;
    delimiter_ = 0;
    quoteChar_ = 0;
    nameValueSeparator_ = 0;
    updateCount_ = 0;
}

//-----------------------------------------------------------------------------

void Strings::error(const char* msg, int data) const
{
    iseThrowException(formatString(msg, data).c_str());
}

//-----------------------------------------------------------------------------

string Strings::extractName(const char* str) const
{
    string result(str);

    string::size_type i = result.find(getNameValueSeparator());
    if (i != string::npos)
        result = result.substr(0, i);
    else
        result.clear();

    return result;
}

//-----------------------------------------------------------------------------

void Strings::assign(const Strings& src)
{
    AutoUpdater autoUpdater(*this);
    clear();

    defined_ = src.defined_;
    delimiter_ = src.delimiter_;
    lineBreak_ = src.lineBreak_;
    quoteChar_ = src.quoteChar_;
    nameValueSeparator_ = src.nameValueSeparator_;

    addStrings(src);
}

///////////////////////////////////////////////////////////////////////////////
// class StrList

//-----------------------------------------------------------------------------

StrList::StrList()
{
    init();
}

//-----------------------------------------------------------------------------

StrList::StrList(const StrList& src)
{
    init();
    assign(src);
}

//-----------------------------------------------------------------------------

StrList::~StrList()
{
    internalClear();
}

//-----------------------------------------------------------------------------

void StrList::assign(const StrList& src)
{
    Strings::operator=(src);
}

//-----------------------------------------------------------------------------

int StrList::add(const char* str)
{
    return add(str, NULL);
}

//-----------------------------------------------------------------------------

int StrList::add(const char* str, POINTER data)
{
    int result;

    if (!isSorted_)
        result = count_;
    else
    {
        if (find(str, result))
            switch (dupMode_)
            {
            case DM_IGNORE:
                return result;
            case DM_ERROR:
                error(SEM_DUPLICATE_STRING, 0);
            default:
                break;
            }
    }

    insertItem(result, str, data);
    return result;
}

//-----------------------------------------------------------------------------

void StrList::clear()
{
    internalClear();
}

//-----------------------------------------------------------------------------

void StrList::del(int index)
{
    if (index < 0 || index >= count_)
        error(SEM_LIST_INDEX_ERROR, index);

    onChanging();

    delete list_[index].str;
    list_[index].str = NULL;

    count_--;
    if (index < count_)
    {
        memmove(list_ + index, list_ + index + 1, (count_ - index) * sizeof(StringItem));
        list_[count_].str = NULL;
    }

    onChanged();
}

//-----------------------------------------------------------------------------

void StrList::exchange(int index1, int index2)
{
    if (index1 < 0 || index1 >= count_)
        error(SEM_LIST_INDEX_ERROR, index1);
    if (index2 < 0 || index2 >= count_)
        error(SEM_LIST_INDEX_ERROR, index2);

    onChanging();
    exchangeItems(index1, index2);
    onChanged();
}

//-----------------------------------------------------------------------------

int StrList::indexOf(const char* str) const
{
    int result;

    if (!isSorted_)
        result = Strings::indexOf(str);
    else if (!find(str, result))
        result = -1;

    return result;
}

//-----------------------------------------------------------------------------

void StrList::insert(int index, const char* str)
{
    insert(index, str, NULL);
}

//-----------------------------------------------------------------------------

void StrList::insert(int index, const char* str, POINTER data)
{
    if (isSorted_)
        error(SEM_SORTED_LIST_ERROR, 0);
    if (index < 0 || index > count_)
        error(SEM_LIST_INDEX_ERROR, index);

    insertItem(index, str, data);
}

//-----------------------------------------------------------------------------

void StrList::setCapacity(int value)
{
    if (value < 0) value = 0;

    for (int i = value; i < capacity_; i++)
        delete list_[i].str;

    if (value > 0)
    {
        StringItem *p = (StringItem*)realloc(list_, value * sizeof(StringItem));
        if (p)
            list_ = p;
        else
            iseThrowMemoryException();
    }
    else
    {
        if (list_)
        {
            free(list_);
            list_ = NULL;
        }
    }

    if (list_ != NULL)
    {
        for (int i = capacity_; i < value; i++)
        {
            list_[i].str = NULL;   // new string object when needed
            list_[i].data = NULL;
        }
    }

    capacity_ = value;
    if (count_ > capacity_)
        count_ = capacity_;
}

//-----------------------------------------------------------------------------

POINTER StrList::getData(int index) const
{
    if (index < 0 || index >= count_)
        error(SEM_LIST_INDEX_ERROR, index);

    return list_[index].data;
}

//-----------------------------------------------------------------------------

void StrList::setData(int index, POINTER data)
{
    if (index < 0 || index >= count_)
        error(SEM_LIST_INDEX_ERROR, index);

    onChanging();
    list_[index].data = data;
    onChanged();
}

//-----------------------------------------------------------------------------

const string& StrList::getString(int index) const
{
    if (index < 0 || index >= count_)
        error(SEM_LIST_INDEX_ERROR, index);

    return stringObjectNeeded(index);
}

//-----------------------------------------------------------------------------

void StrList::setString(int index, const char* value)
{
    if (isSorted_)
        error(SEM_SORTED_LIST_ERROR, 0);
    if (index < 0 || index >= count_)
        error(SEM_LIST_INDEX_ERROR, index);

    onChanging();
    stringObjectNeeded(index) = value;
    onChanged();
}

//-----------------------------------------------------------------------------

bool StrList::find(const char* str, int& index) const
{
    if (!isSorted_)
    {
        index = indexOf(str);
        return (index >= 0);
    }

    bool result = false;
    int l, h, i, c;

    l = 0;
    h = count_ - 1;
    while (l <= h)
    {
        i = (l + h) >> 1;
        c = compareStrings(stringObjectNeeded(i).c_str(), str);
        if (c < 0)
            l = i + 1;
        else
        {
            h = i - 1;
            if (c == 0)
            {
                result = true;
                if (dupMode_ != DM_ACCEPT)
                    l = i;
            }
        }
    }

    index = l;
    return result;
}

//-----------------------------------------------------------------------------

int stringListCompareProc(const StrList& stringList, int index1, int index2)
{
    return stringList.compareStrings(
        stringList.stringObjectNeeded(index1).c_str(),
        stringList.stringObjectNeeded(index2).c_str());
}

//-----------------------------------------------------------------------------

void StrList::sort()
{
    sort(stringListCompareProc);
}

//-----------------------------------------------------------------------------

void StrList::sort(StringListCompareProc compareProc)
{
    if (!isSorted_ && count_ > 1)
    {
        onChanging();
        quickSort(0, count_ - 1, compareProc);
        onChanged();
    }
}

//-----------------------------------------------------------------------------

void StrList::setSorted(bool value)
{
    if (value != isSorted_)
    {
        if (value) sort();
        isSorted_ = value;
    }
}

//-----------------------------------------------------------------------------

void StrList::setCaseSensitive(bool value)
{
    if (value != isCaseSensitive_)
    {
        isCaseSensitive_ = value;
        if (isSorted_) sort();
    }
}

//-----------------------------------------------------------------------------

StrList& StrList::operator = (const StrList& rhs)
{
    if (this != &rhs)
        assign(rhs);
    return *this;
}

//-----------------------------------------------------------------------------

void StrList::setUpdateState(bool isUpdating)
{
    if (isUpdating)
        onChanging();
    else
        onChanged();
}

//-----------------------------------------------------------------------------

int StrList::compareStrings(const char* str1, const char* str2) const
{
    if (isCaseSensitive_)
        return strcmp(str1, str2);
    else
        return compareText(str1, str2);
}

//-----------------------------------------------------------------------------

void StrList::insertItem(int index, const char* str, POINTER data)
{
    onChanging();
    if (count_ == capacity_) grow();
    if (index < count_)
    {
        memmove(list_ + index + 1, list_ + index, (count_ - index) * sizeof(StringItem));
        list_[index].str = NULL;
    }

    stringObjectNeeded(index) = str;
    list_[index].data = data;

    count_++;
    onChanged();
}

//-----------------------------------------------------------------------------

void StrList::init()
{
    list_ = NULL;
    count_ = 0;
    capacity_ = 0;
    dupMode_ = DM_IGNORE;
    isSorted_ = false;
    isCaseSensitive_ = false;
}

//-----------------------------------------------------------------------------

void StrList::internalClear()
{
    setCapacity(0);
}

//-----------------------------------------------------------------------------

string& StrList::stringObjectNeeded(int index) const
{
    if (list_[index].str == NULL)
        list_[index].str = new string();
    return *(list_[index].str);
}

//-----------------------------------------------------------------------------

void StrList::exchangeItems(int index1, int index2)
{
    StringItem temp;

    temp = list_[index1];
    list_[index1] = list_[index2];
    list_[index2] = temp;
}

//-----------------------------------------------------------------------------

void StrList::grow()
{
    int delta;

    if (capacity_ > 64)
        delta = capacity_ / 4;
    else if (capacity_ > 8)
        delta = 16;
    else
        delta = 4;

    setCapacity(capacity_ + delta);
}

//-----------------------------------------------------------------------------

void StrList::quickSort(int l, int r, StringListCompareProc compareProc)
{
    int i, j, p;

    do
    {
        i = l;
        j = r;
        p = (l + r) >> 1;
        do
        {
            while (compareProc(*this, i, p) < 0) i++;
            while (compareProc(*this, j, p) > 0) j--;
            if (i <= j)
            {
                exchangeItems(i, j);
                if (p == i)
                    p = j;
                else if (p == j)
                    p = i;
                i++;
                j--;
            }
        }
        while (i <= j);

        if (l < j)
            quickSort(l, j, compareProc);
        l = i;
    }
    while (i < r);
}

///////////////////////////////////////////////////////////////////////////////
// class Url

Url::Url(const string& url)
{
    setUrl(url);
}

Url::Url(const Url& src)
{
    (*this) = src;
}

//-----------------------------------------------------------------------------

void Url::clear()
{
    protocol_.clear();
    host_.clear();
    port_.clear();
    path_.clear();
    fileName_.clear();
    bookmark_.clear();
    userName_.clear();
    password_.clear();
    params_.clear();
}

//-----------------------------------------------------------------------------

Url& Url::operator = (const Url& rhs)
{
    if (this == &rhs) return *this;

    protocol_ = rhs.protocol_;
    host_ = rhs.host_;
    port_ = rhs.port_;
    path_ = rhs.path_;
    fileName_ = rhs.fileName_;
    bookmark_ = rhs.bookmark_;
    userName_ = rhs.userName_;
    password_ = rhs.password_;
    params_ = rhs.params_;

    return *this;
}

//-----------------------------------------------------------------------------

string Url::getUrl() const
{
    const char SEP_CHAR = '/';
    string result;

    if (!protocol_.empty())
        result = protocol_ + "://";

    if (!userName_.empty())
    {
        result += userName_;
        if (!password_.empty())
            result = result + ":" + password_;
        result += "@";
    }

    result += host_;

    if (!port_.empty())
    {
        if (sameText(protocol_, "HTTP"))
        {
            if (port_ != "80")
                result = result + ":" + port_;
        }
        else if (sameText(protocol_, "HTTPS"))
        {
            if (port_ != "443")
                result = result + ":" + port_;
        }
        else if (sameText(protocol_, "FTP"))
        {
            if (port_ != "21")
                result = result + ":" + port_;
        }
    }

    // path and filename
    string str = path_;
    if (!str.empty() && str[str.length()-1] != SEP_CHAR)
        str += SEP_CHAR;
    str += fileName_;

    if (!str.empty())
    {
        if (!result.empty() && str[0] == SEP_CHAR) str.erase(0, 1);
        if (!host_.empty() && result[result.length()-1] != SEP_CHAR)
            result += SEP_CHAR;
        result += str;
    }

    if (!params_.empty())
        result = result + "?" + params_;

    if (!bookmark_.empty())
        result = result + "#" + bookmark_;

    return result;
}

//-----------------------------------------------------------------------------

string Url::getUrl(UINT parts)
{
    Url url(*this);

    if (!(parts & URL_PROTOCOL)) url.setProtocol("");
    if (!(parts & URL_HOST)) url.setHost("");
    if (!(parts & URL_PORT)) url.setPort("");
    if (!(parts & URL_PATH)) url.setPath("");
    if (!(parts & URL_FILENAME)) url.setFileName("");
    if (!(parts & URL_BOOKMARK)) url.setBookmark("");
    if (!(parts & URL_USERNAME)) url.setUserName("");
    if (!(parts & URL_PASSWORD)) url.setPassword("");
    if (!(parts & URL_PARAMS)) url.setParams("");

    return url.getUrl();
}

//-----------------------------------------------------------------------------

void Url::setUrl(const string& value)
{
    clear();

    string url(value);
    if (url.empty()) return;

    // get the bookmark
    string::size_type pos = url.rfind('#');
    if (pos != string::npos)
    {
        bookmark_ = url.substr(pos + 1);
        url.erase(pos);
    }

    // get the parameters
    pos = url.find('?');
    if (pos != string::npos)
    {
        params_ = url.substr(pos + 1);
        url = url.substr(0, pos);
    }

    string buffer;
    pos = url.find("://");
    if (pos != string::npos)
    {
        protocol_ = url.substr(0, pos);
        url.erase(0, pos + 3);
        // get the user name, password, host and the port number
        buffer = fetchStr(url, '/', true);
        // get username and password
        pos = buffer.find('@');
        if (pos != string::npos)
        {
            password_ = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            userName_ = fetchStr(password_, ':');
            if (userName_.empty())
                password_.clear();
        }
        // get the host and the port number
        string::size_type p1, p2;
        if ((p1 = buffer.find('[')) != string::npos &&
            (p2 = buffer.find(']')) != string::npos &&
            p2 > p1)
        {
            // this is for IPv6 Hosts
            host_ = fetchStr(buffer, ']');
            fetchStr(host_, '[');
            fetchStr(buffer, ':');
        }
        else
            host_ = fetchStr(buffer, ':', true);
        port_ = buffer;
        // get the path
        pos = url.rfind('/');
        if (pos != string::npos)
        {
            path_ = "/" + url.substr(0, pos + 1);
            url.erase(0, pos + 1);
        }
        else
            path_ = "/";
    }
    else
    {
        // get the path
        pos = url.rfind('/');
        if (pos != string::npos)
        {
            path_ = url.substr(0, pos + 1);
            url.erase(0, pos + 1);
        }
    }

    // get the filename
    fileName_ = url;
}

///////////////////////////////////////////////////////////////////////////////
// class Packet

Packet::Packet()
{
    init();
}

//-----------------------------------------------------------------------------

Packet::~Packet()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 数据打包
// 返回:
//   true  - 成功
//   false - 失败
// 备注:
//   打包后的数据可由 Packet.getBuffer 和 Packet.getSize 取出。
//-----------------------------------------------------------------------------
bool Packet::pack()
{
    bool result;

    try
    {
        delete stream_;
        stream_ = new MemoryStream(DEFAULT_MEMORY_DELTA);
        doPack();
        doAfterPack();
        doCompress();
        doEncrypt();
        isAvailable_ = true;
        isPacked_ = true;
        result = true;
    }
    catch (Exception&)
    {
        result = false;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 数据解包
// 返回:
//   true  - 成功
//   false - 失败 (数据不足、格式错误等)
//-----------------------------------------------------------------------------
bool Packet::unpack(void *buffer, int bytes)
{
    bool result;

    try
    {
        delete stream_;
        stream_ = new MemoryStream(DEFAULT_MEMORY_DELTA);
        stream_->setSize(bytes);
        memmove(stream_->getMemory(), buffer, bytes);
        doDecrypt();
        doDecompress();
        doUnpack();
        isAvailable_ = true;
        isPacked_ = false;
        result = true;
    }
    catch (Exception&)
    {
        result = false;
        clear();
    }

    delete stream_;
    stream_ = NULL;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 数据解包
//-----------------------------------------------------------------------------
bool Packet::unpack(const Buffer& buffer)
{
    return unpack(buffer.data(), buffer.getSize());
}

//-----------------------------------------------------------------------------
// 描述: 清空数据包内容
//-----------------------------------------------------------------------------
void Packet::clear()
{
    delete stream_;
    stream_ = NULL;
    isAvailable_ = false;
    isPacked_ = false;
}

//-----------------------------------------------------------------------------
// 描述: 确保数据已打包
//-----------------------------------------------------------------------------
void Packet::ensurePacked()
{
    if (!IsPacked()) pack();
}

//-----------------------------------------------------------------------------

void Packet::throwUnpackError()
{
    iseThrowException(SEM_PACKET_UNPACK_ERROR);
}

//-----------------------------------------------------------------------------

void Packet::throwPackError()
{
    iseThrowException(SEM_PACKET_PACK_ERROR);
}

//-----------------------------------------------------------------------------

void Packet::checkUnsafeSize(int value)
{
    const int MAX_UNSAFE_SIZE = 1024*1024*8;  // 8M

    if (value < 0 || value > MAX_UNSAFE_SIZE)
        iseThrowException(SEM_UNSAFE_VALUE_IN_PACKET);
}

//-----------------------------------------------------------------------------

void Packet::readBuffer(void *buffer, int bytes)
{
    if (stream_->read(buffer, bytes) != bytes)
        throwUnpackError();
}

//-----------------------------------------------------------------------------

void Packet::readString(std::string& str)
{
    DWORD size;

    str.clear();
    if (stream_->read(&size, sizeof(DWORD)) == sizeof(DWORD))
    {
        checkUnsafeSize(size);
        if (size > 0)
        {
            str.resize(size);
            if (stream_->read((void*)str.data(), size) != size)
                throwUnpackError();
        }
    }
    else
    {
        throwUnpackError();
    }
}

//-----------------------------------------------------------------------------

void Packet::readBlob(std::string& str)
{
    readString(str);
}

//-----------------------------------------------------------------------------

void Packet::readBlob(Stream& stream)
{
    std::string str;

    readBlob(str);
    stream.setSize(0);
    if (!str.empty())
        stream.write((void*)str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------

void Packet::readBlob(Buffer& buffer)
{
    std::string str;

    readBlob(str);
    buffer.setSize(0);
    if (!str.empty())
        buffer.assign((void*)str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------

void Packet::writeBuffer(const void *buffer, int bytes)
{
    ISE_ASSERT(buffer && bytes >= 0);
    stream_->write(buffer, bytes);
}

//-----------------------------------------------------------------------------

void Packet::writeString(const std::string& str)
{
    DWORD size;

    size = (DWORD)str.length();
    stream_->write(&size, sizeof(DWORD));
    if (size > 0)
        stream_->write((void*)str.c_str(), size);
}

//-----------------------------------------------------------------------------

void Packet::writeBlob(void *buffer, int bytes)
{
    DWORD size = 0;

    if (buffer && bytes >= 0)
        size = (DWORD)bytes;
    stream_->write(&size, sizeof(DWORD));
    if (size > 0)
        stream_->write(buffer, size);
}

//-----------------------------------------------------------------------------

void Packet::writeBlob(const Buffer& buffer)
{
    writeBlob(buffer.data(), buffer.getSize());
}

//-----------------------------------------------------------------------------
// 描述: 固定字符串的长度
//-----------------------------------------------------------------------------
void Packet::fixStrLength(std::string& str, int length)
{
    if ((int)str.length() != length)
        str.resize(length, 0);
}

//-----------------------------------------------------------------------------
// 描述: 限制字符串的最大长度
//-----------------------------------------------------------------------------
void Packet::truncString(std::string& str, int maxLength)
{
    if ((int)str.length() > maxLength)
        str.resize(maxLength);
}

//-----------------------------------------------------------------------------

void Packet::init()
{
    stream_ = NULL;
    isAvailable_ = false;
    isPacked_ = false;
}

///////////////////////////////////////////////////////////////////////////////
// class Logger

Logger::Logger() :
    isNewFileDaily_(false)
{
    // nothing
}

//-----------------------------------------------------------------------------

Logger& Logger::instance()
{
    static Logger obj;
    return obj;
}

//-----------------------------------------------------------------------------
// 描述: 设置日志文件名
// 参数:
//   fileName   - 日志文件名 (含路径)
//   isNewFileDaily - 如果为true，将会自动在文件名后(后缀名前)加上当天的日期
//-----------------------------------------------------------------------------
void Logger::setFileName(const string& fileName, bool isNewFileDaily)
{
    fileName_ = fileName;
    isNewFileDaily_ = isNewFileDaily;
}

//-----------------------------------------------------------------------------
// 描述: 将文本写入日志
//-----------------------------------------------------------------------------
void Logger::writeStr(const char *str)
{
    string text;
    UINT processId, threadId;

#ifdef ISE_WINDOWS
    processId = GetCurrentProcessId();
    threadId = GetCurrentThreadId();
#endif
#ifdef ISE_LINUX
    processId = getpid();
    threadId = pthread_self();
#endif

    text = formatString("[%s](%05d|%05u)<%s>\n",
        DateTime::currentDateTime().dateTimeString().c_str(),
        processId, threadId, str);

    writeToFile(text);
}

//-----------------------------------------------------------------------------
// 描述: 将文本写入日志
//-----------------------------------------------------------------------------
void Logger::writeFmt(const char *format, ...)
{
    string text;

    va_list argList;
    va_start(argList, format);
    formatStringV(text, format, argList);
    va_end(argList);

    writeStr(text.c_str());
}

//-----------------------------------------------------------------------------
// 描述: 将异常信息写入日志
//-----------------------------------------------------------------------------
void Logger::writeException(const Exception& e)
{
    writeStr(e.makeLogStr().c_str());
}

//-----------------------------------------------------------------------------

string Logger::getLogFileName()
{
    string result = fileName_;

    if (result.empty())
        result = getAppPath() + "log.txt";

    if (isNewFileDaily_)
    {
        string fileExt = extractFileExt(result);
        result = result.substr(0, result.length() - fileExt.length()) + ".";
        result += DateTime::currentDateTime().dateString("");
        result += fileExt;
    }

    return result;
}

//-----------------------------------------------------------------------------

bool Logger::openFile(FileStream& fileStream, const string& fileName)
{
    return
        fileStream.open(fileName, FM_OPEN_WRITE | FM_SHARE_DENY_NONE) ||
        fileStream.open(fileName, FM_CREATE | FM_SHARE_DENY_NONE);
}

//-----------------------------------------------------------------------------
// 描述: 将字符串写入文件
//-----------------------------------------------------------------------------
void Logger::writeToFile(const string& str)
{
    AutoLocker locker(lock_);

    string fileName = getLogFileName();
    FileStream fs;
    if (!openFile(fs, fileName))
    {
        string path = extractFilePath(fileName);
        if (!path.empty())
        {
            forceDirectories(path);
            openFile(fs, fileName);
        }
    }

    if (fs.isOpen())
    {
        fs.seek(0, SO_END);
        fs.write(str.c_str(), (int)str.length());
    }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
