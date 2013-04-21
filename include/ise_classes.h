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
// ise_classes.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_CLASSES_H_
#define _ISE_CLASSES_H_

#include "ise_options.h"

#ifdef ISE_WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/timeb.h>
#include <sys/file.h>
#include <iostream>
#include <fstream>
#include <string>
#endif

#include "ise_global_defs.h"
#include "ise_errmsgs.h"
#include "ise_exceptions.h"

using namespace std;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class declares

class Buffer;
class DateTime;
class AutoInvoker;
class AutoInvokable;
class CriticalSection;
class Semaphore;
class SignalMasker;
class SeqNumberAlloc;
class Stream;
class MemoryStream;
class FileStream;
class PointerList;
class PropertyList;
class Strings;
class StrList;
class Url;
class Packet;
template<typename ObjectType> class CustomObjectList;
template<typename ObjectType> class ObjectList;
template<typename T> class CallbackList;
class ObjectContext;
class Logger;

///////////////////////////////////////////////////////////////////////////////
// class Buffer - 缓存类

class Buffer
{
public:
    Buffer();
    Buffer(const Buffer& src);
    explicit Buffer(int size);
    Buffer(const void *buffer, int size);
    virtual ~Buffer();

    Buffer& operator = (const Buffer& rhs);
    const char& operator[] (int index) const { return ((char*)buffer_)[index]; }
    char& operator[] (int index) { return const_cast<char&>(((const Buffer&)(*this))[index]); }
    operator char*() const { return (char*)buffer_; }
    char* data() const { return (char*)buffer_; }
    char* c_str() const;
    void assign(const void *buffer, int size);
    void clear() { setSize(0); }
    void setSize(int size, bool initZero = false);
    int getSize() const { return size_; }
    void ensureSize(int size) { if (getSize() < size) setSize(size); }
    void setPosition(int position);
    int getPosition() const { return position_; }

    bool loadFromStream(Stream& stream);
    bool loadFromFile(const string& fileName);
    bool saveToStream(Stream& stream);
    bool saveToFile(const string& fileName);
private:
    inline void init() { buffer_ = NULL; size_ = 0; position_ = 0; }
    void assign(const Buffer& src);
    void verifyPosition();
protected:
    void *buffer_;
    int size_;
    int position_;
};

///////////////////////////////////////////////////////////////////////////////
// class DateTime - 日期时间类

class DateTime
{
public:
    DateTime() { time_ = 0; }
    DateTime(const DateTime& src) { time_ = src.time_; }
    explicit DateTime(time_t src) { time_ = src; }
    explicit DateTime(const string& src) { *this = src; }

    static DateTime currentDateTime();

    DateTime& operator = (const DateTime& rhs)
        { time_ = rhs.time_; return *this; }
    DateTime& operator = (const time_t rhs)
        { time_ = rhs; return *this; }
    DateTime& operator = (const string& dateTimeStr);

    DateTime operator + (const DateTime& rhs) const { return DateTime(time_ + rhs.time_); }
    DateTime operator + (time_t rhs) const { return DateTime(time_ + rhs); }
    DateTime operator - (const DateTime& rhs) const { return DateTime(time_ - rhs.time_); }
    DateTime operator - (time_t rhs) const { return DateTime(time_ - rhs); }

    bool operator == (const DateTime& rhs) const { return time_ == rhs.time_; }
    bool operator != (const DateTime& rhs) const { return time_ != rhs.time_; }
    bool operator > (const DateTime& rhs) const  { return time_ > rhs.time_; }
    bool operator < (const DateTime& rhs) const  { return time_ < rhs.time_; }
    bool operator >= (const DateTime& rhs) const { return time_ >= rhs.time_; }
    bool operator <= (const DateTime& rhs) const { return time_ <= rhs.time_; }

    operator time_t() const { return time_; }

    void encodeDateTime(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0);
    void decodeDateTime(int *year, int *month, int *day,
        int *hour, int *minute, int *second,
        int *weekDay = NULL, int *yearDay = NULL) const;

    string dateString(const string& dateSep = "-") const;
    string dateTimeString(const string& dateSep = "-",
        const string& dateTimeSep = " ", const string& timeSep = ":") const;

private:
    time_t time_;     // (从1970-01-01 00:00:00 算起的秒数，UTC时间)
};

///////////////////////////////////////////////////////////////////////////////
// class AutoFinalizer - 基于作用域的自动析构器

class AutoFinalizer : boost::noncopyable
{
public:
    typedef boost::function<void (void)> Finalizer;
public:
    AutoFinalizer(const Finalizer& f) : f_(f) {}
    ~AutoFinalizer() { f_(); }
private:
    Finalizer f_;
};

///////////////////////////////////////////////////////////////////////////////
// class AutoInvokable/AutoInvoker - 自动被调对象/自动调用者
//
// 说明:
// 1. 这两个类联合使用，可以起到和 "智能指针" 类似的作用，即利用栈对象自动销毁的特性，在栈
//    对象的生命周期中自动调用 AutoInvokable::invokeInitialize() 和 invokeFinalize()。
//    此二类一般使用在重要资源的对称性操作场合(比如加锁/解锁)。
// 2. 使用者需继承 AutoInvokable 类，重写 invokeInitialize() 和 invokeFinalize()
//    函数。并在需要调用的地方定义 AutoInvoker 的栈对象。

class AutoInvokable
{
public:
    friend class AutoInvoker;
    virtual ~AutoInvokable() {}
protected:
    virtual void invokeInitialize() {}
    virtual void invokeFinalize() {}
};

class AutoInvoker : boost::noncopyable
{
public:
    explicit AutoInvoker(AutoInvokable& object)
        { object_ = &object; object_->invokeInitialize(); }

    explicit AutoInvoker(AutoInvokable *object)
        { object_ = object; if (object_) object_->invokeInitialize(); }

    virtual ~AutoInvoker()
        { if (object_) object_->invokeFinalize(); }

private:
    AutoInvokable *object_;
};

///////////////////////////////////////////////////////////////////////////////
// class AutoLocker - 线程自动互斥类
//
// 说明:
// 1. 此类利用C++的栈对象自动销毁的特性，在多线程环境下进行局部范围临界区互斥；
// 2. 使用方法: 在需要互斥的范围中以局部变量方式定义此类对象即可；
//
// 使用范例:
//   假设已定义: CriticalSection lock_;
//   自动加锁和解锁:
//   {
//       AutoLocker locker(lock_);
//       //...
//   }

typedef AutoInvoker AutoLocker;

///////////////////////////////////////////////////////////////////////////////
// class CriticalSection - 线程临界区互斥类
//
// 说明:
// 1. 此类用于多线程环境下临界区互斥，基本操作有 lock、unlock 和 tryLock；
// 2. 线程内允许嵌套调用 lock，嵌套调用后必须调用相同次数的 unlock 才可解锁；

class CriticalSection :
    public AutoInvokable,
    boost::noncopyable
{
public:
    CriticalSection();
    virtual ~CriticalSection();

    // 加锁
    void lock();
    // 解锁
    void unlock();
    // 尝试加锁 (若已经处于加锁状态则立即返回 false)
    bool tryLock();

protected:
    virtual void invokeInitialize() { lock(); }
    virtual void invokeFinalize() { unlock(); }

private:
#ifdef ISE_WINDOWS
    CRITICAL_SECTION lock_;
#endif
#ifdef ISE_LINUX
    pthread_mutex_t lock_;
#endif
};

///////////////////////////////////////////////////////////////////////////////
// class Semaphore - 线程旗标类

class Semaphore : boost::noncopyable
{
public:
    explicit Semaphore(UINT initValue = 0);
    virtual ~Semaphore();

    void increase();
    void wait();
    void reset();

private:
    void doCreateSem();
    void doDestroySem();

private:
#ifdef ISE_WINDOWS
    HANDLE sem_;
#endif
#ifdef ISE_LINUX
    sem_t sem_;
#endif

    UINT initValue_;
};

///////////////////////////////////////////////////////////////////////////////
// class SignalMasker - 信号屏蔽类

#ifdef ISE_LINUX
class SignalMasker : boost::noncopyable
{
public:
    explicit SignalMasker(bool isAutoRestore = false);
    virtual ~SignalMasker();

    // 设置 Block/UnBlock 操作所需的信号集合
    void setSignals(int sigCount, ...);
    void setSignals(int sigCount, va_list argList);

    // 在进程当前阻塞信号集中添加 setSignals 设置的信号
    void block();
    // 在进程当前阻塞信号集中解除 setSignals 设置的信号
    void unBlock();

    // 将进程阻塞信号集恢复为 Block/UnBlock 之前的状态
    void restore();

private:
    int sigProcMask(int how, const sigset_t *newSet, sigset_t *oldSet);

private:
    sigset_t oldSet_;
    sigset_t newSet_;
    bool isBlock_;
    bool isAutoRestore_;
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class AtomicInteger - 提供原子操作的整数类

#ifdef ISE_WINDOWS

class AtomicInt : boost::noncopyable
{
public:
    AtomicInt() : value_(0) {}

    LONG get() { return InterlockedCompareExchange(&value_, 0, 0); }
    LONG set(LONG newValue) { return InterlockedExchange(&value_, newValue); }
    LONG getAndAdd(LONG x) { return InterlockedExchangeAdd(&value_, x); }
    LONG addAndGet(LONG x) { return getAndAdd(x) + x; }
    LONG increment() { return InterlockedIncrement(&value_); }
    LONG decrement() { return InterlockedDecrement(&value_); }
    LONG getAndSet(LONG newValue) { return set(newValue); }

private:
    volatile LONG value_;
};

class AtomicInt64 : boost::noncopyable
{
public:
    AtomicInt64() : value_(0) {}

    INT64 get()
    {
        AutoLocker locker(lock_);
        return value_;
    }
    INT64 set(INT64 newValue)
    {
        AutoLocker locker(lock_);
        INT64 temp = value_;
        value_ = newValue;
        return temp;
    }
    INT64 getAndAdd(INT64 x)
    {
        AutoLocker locker(lock_);
        INT64 temp = value_;
        value_ += x;
        return temp;
    }
    INT64 addAndGet(INT64 x)
    {
        AutoLocker locker(lock_);
        value_ += x;
        return value_;
    }
    INT64 increment()
    {
        AutoLocker locker(lock_);
        ++value_;
        return value_;
    }
    INT64 decrement()
    {
        AutoLocker locker(lock_);
        --value_;
        return value_;
    }
    INT64 getAndSet(INT64 newValue)
    {
        return set(newValue);
    }

private:
    volatile INT64 value_;
    CriticalSection lock_;
};

#endif

#ifdef ISE_LINUX

template<typename T>
class AtomicInteger : boost::noncopyable
{
public:
    AtomicInteger() : value_(0) {}

    T get() { return __sync_val_compare_and_swap(&value_, 0, 0); }
    T set(T newValue) { return __sync_lock_test_and_set(&value_, newValue); }
    T getAndAdd(T x) { return __sync_fetch_and_add(&value_, x); }
    T addAndGet(T x) { return __sync_add_and_fetch(&value_, x); }
    T increment() { return addAndGet(1); }
    T decrement() { return addAndGet(-1); }
    T getAndSet(T newValue) { return set(newValue); }

private:
    volatile T value_;
};

typedef AtomicInteger<long> AtomicInt;
typedef AtomicInteger<INT64> AtomicInt64;

#endif

///////////////////////////////////////////////////////////////////////////////
// class SeqNumberAlloc - 整数序列号分配器类
//
// 说明:
// 1. 此类以线程安全方式生成一个不断递增的整数序列，用户可以指定序列的起始值；
// 2. 此类一般用于数据包的顺序号控制；

class SeqNumberAlloc : boost::noncopyable
{
public:
    explicit SeqNumberAlloc(UINT64 startId = 0);

    // 返回一个新分配的ID
    UINT64 allocId();

private:
    CriticalSection lock_;
    UINT64 currentId_;
};

///////////////////////////////////////////////////////////////////////////////
// class Stream - 流 基类

enum SEEK_ORIGIN
{
    SO_BEGINNING    = 0,
    SO_CURRENT      = 1,
    SO_END          = 2
};

class Stream
{
public:
    virtual ~Stream() {}

    virtual int read(void *buffer, int count) = 0;
    virtual int write(const void *buffer, int count) = 0;
    virtual INT64 seek(INT64 offset, SEEK_ORIGIN seekOrigin) = 0;

    void readBuffer(void *buffer, int count);
    void writeBuffer(const void *buffer, int count);

    INT64 getPosition() { return seek(0, SO_CURRENT); }
    void setPosition(INT64 pos) { seek(pos, SO_BEGINNING); }

    virtual INT64 getSize();
    virtual void setSize(INT64 size) {}
};

///////////////////////////////////////////////////////////////////////////////
// class MemoryStream - 内存流类

class MemoryStream : public Stream
{
public:
    enum { DEFAULT_MEMORY_DELTA = 1024 };    // 缺省内存增长步长 (字节数，必须是 2 的 N 次方)
    enum { MIN_MEMORY_DELTA = 256 };         // 最小内存增长步长

public:
    explicit MemoryStream(int memoryDelta = DEFAULT_MEMORY_DELTA);
    virtual ~MemoryStream();

    virtual int read(void *buffer, int count);
    virtual int write(const void *buffer, int count);
    virtual INT64 seek(INT64 offset, SEEK_ORIGIN seekOrigin);
    virtual void setSize(INT64 size);
    bool loadFromStream(Stream& stream);
    bool loadFromFile(const string& fileName);
    bool saveToStream(Stream& stream);
    bool saveToFile(const string& fileName);
    void clear();
    char* getMemory() { return memory_; }

private:
    void setMemoryDelta(int newMemoryDelta);
    void setPointer(char *memory, int size);
    void setCapacity(int newCapacity);
    char* realloc(int& newCapacity);

private:
    char *memory_;
    int capacity_;
    int size_;
    int position_;
    int memoryDelta_;
};

///////////////////////////////////////////////////////////////////////////////
// class FileStream - 文件流类

// 文件流打开方式 (UINT openMode)
#ifdef ISE_WINDOWS
enum
{
    FM_CREATE           = 0xFFFF,
    FM_OPEN_READ        = 0x0000,
    FM_OPEN_WRITE       = 0x0001,
    FM_OPEN_READ_WRITE  = 0x0002,

    FM_SHARE_EXCLUSIVE  = 0x0010,
    FM_SHARE_DENY_WRITE = 0x0020,
    FM_SHARE_DENY_NONE  = 0x0040
};
#endif
#ifdef ISE_LINUX
enum
{
    FM_CREATE           = 0xFFFF,
    FM_OPEN_READ        = O_RDONLY,  // 0
    FM_OPEN_WRITE       = O_WRONLY,  // 1
    FM_OPEN_READ_WRITE  = O_RDWR,    // 2

    FM_SHARE_EXCLUSIVE  = 0x0010,
    FM_SHARE_DENY_WRITE = 0x0020,
    FM_SHARE_DENY_NONE  = 0x0030
};
#endif

// 缺省文件存取权限 (rights)
#ifdef ISE_WINDOWS
enum { DEFAULT_FILE_ACCESS_RIGHTS = 0 };
#endif
#ifdef ISE_LINUX
enum { DEFAULT_FILE_ACCESS_RIGHTS = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH };
#endif

class FileStream :
    public Stream,
    boost::noncopyable
{
public:
    FileStream();
    FileStream(const string& fileName, UINT openMode, UINT rights = DEFAULT_FILE_ACCESS_RIGHTS);
    virtual ~FileStream();

    bool open(const string& fileName, UINT openMode,
        UINT rights = DEFAULT_FILE_ACCESS_RIGHTS, FileException* exception = NULL);
    void close();

    virtual int read(void *buffer, int count);
    virtual int write(const void *buffer, int count);
    virtual INT64 seek(INT64 offset, SEEK_ORIGIN seekOrigin);
    virtual void setSize(INT64 size);

    string getFileName() const { return fileName_; }
    HANDLE getHandle() const { return handle_; }
    bool isOpen() const;

private:
    void init();
    HANDLE fileCreate(const string& fileName, UINT rights);
    HANDLE fileOpen(const string& fileName, UINT openMode);
    void fileClose(HANDLE handle);
    int fileRead(HANDLE handle, void *buffer, int count);
    int fileWrite(HANDLE handle, const void *buffer, int count);
    INT64 fileSeek(HANDLE handle, INT64 offset, SEEK_ORIGIN seekOrigin);

private:
    string fileName_;
    HANDLE handle_;
};

///////////////////////////////////////////////////////////////////////////////
// class PointerList - 列表类
//
// 说明:
// 1. 此类的实现原理与 Delphi::TList 完全相同；
// 2. 此列表类有如下优点:
//    a. 公有方法简单明确 (STL虽无比强大但稍显晦涩)；
//    b. 支持下标随机存取各个元素 (STL::list不支持)；
//    c. 支持快速获取列表长度 (STL::list不支持)；
//    d. 支持尾部快速增删元素；
// 3. 此列表类有如下缺点:
//    a. 不支持头部和中部的快速增删元素；
//    b. 只支持单一类型元素(Pointer类型)；

class PointerList
{
public:
    PointerList();
    virtual ~PointerList();

    int add(POINTER item);
    void insert(int index, POINTER item);
    void del(int index);
    int remove(POINTER item);
    POINTER extract(POINTER item);
    void move(int curIndex, int newIndex);
    void resize(int count);
    void clear();

    POINTER first() const;
    POINTER last() const;
    int indexOf(POINTER item) const;
    int getCount() const;
    bool isEmpty() const { return (getCount() <= 0); }

    PointerList& operator = (const PointerList& rhs);
    const POINTER& operator[] (int index) const;
    POINTER& operator[] (int index);

protected:
    virtual void grow();

    POINTER get(int index) const;
    void put(int index, POINTER item);
    void setCapacity(int newCapacity);
    void setCount(int newCount);

private:
    POINTER *list_;
    int count_;
    int capacity_;
};

///////////////////////////////////////////////////////////////////////////////
// class PropertyList - 属性列表类
//
// 说明:
// 1. 属性列表中的每个项目由属性名(Name)和属性值(Value)组成。
// 2. 属性名不可重复，不区分大小写，且其中不可含有等号"="。属性值可为任意值。

class PropertyList
{
public:
    enum { NAME_VALUE_SEP = '=' };        // Name 和 Value 之间的分隔符
    enum { PROP_ITEM_SEP  = ',' };        // 属性项目之间的分隔符
    enum { PROP_ITEM_QUOT = '"' };

    struct CPropertyItem
    {
        string name, value;
    public:
        CPropertyItem(const CPropertyItem& src) :
            name(src.name), value(src.value) {}
        CPropertyItem(const string& _name, const string& _value) :
            name(_name), value(_value) {}
    };

public:
    PropertyList();
    virtual ~PropertyList();

    void add(const string& name, const string& value);
    bool remove(const string& name);
    void clear();
    int indexOf(const string& name) const;
    bool nameExists(const string& name) const;
    bool getValue(const string& name, string& value) const;
    int getCount() const { return items_.getCount(); }
    bool isEmpty() const { return (getCount() <= 0); }
    const CPropertyItem& getItems(int index) const;
    string getPropString() const;
    void setPropString(const string& propString);

    PropertyList& operator = (const PropertyList& rhs);
    string& operator[] (const string& name);

private:
    CPropertyItem* find(const string& name);
    static bool isReservedChar(char ch);
    static bool hasReservedChar(const string& str);
    static char* scanStr(char *str, char ch);
    static string makeQuotedStr(const string& str);
    static string extractQuotedStr(char*& strPtr);

private:
    PointerList items_;                        // (CPropertyItem* [])
};

///////////////////////////////////////////////////////////////////////////////
// class Strings - 字符串列表抽象类

class Strings
{
private:
    enum STRINGS_DEFINED
    {
        SD_DELIMITER         = 0x0001,
        SD_QUOTE_CHAR        = 0x0002,
        SD_NAME_VALUE_SEP    = 0x0004,
        SD_LINE_BREAK        = 0x0008
    };

public:
    // Calls beginUpdate() and endUpdate() automatically in a scope.
    class AutoUpdater
    {
    private:
        Strings& strings;
    public:
        AutoUpdater(Strings& _strings) : strings(_strings)
            { strings.beginUpdate(); }
        ~AutoUpdater()
            { strings.endUpdate(); }
    };

public:
    Strings();
    virtual ~Strings() {}

    virtual int add(const char* str);
    virtual int add(const char* str, POINTER data);
    virtual void addStrings(const Strings& strings);
    virtual void insert(int index, const char* str) = 0;
    virtual void insert(int index, const char* str, POINTER data);
    virtual void clear() = 0;
    virtual void del(int index) = 0;
    virtual bool equals(const Strings& strings);
    virtual void exchange(int index1, int index2);
    virtual void move(int curIndex, int newIndex);
    virtual bool exists(const char* str) const;
    virtual int indexOf(const char* str) const;
    virtual int indexOfName(const char* name) const;
    virtual int indexOfData(POINTER data) const;

    virtual bool loadFromStream(Stream& stream);
    virtual bool loadFromFile(const char* fileName);
    virtual bool saveToStream(Stream& stream) const;
    virtual bool saveToFile(const char* fileName) const;

    virtual int getCapacity() const { return getCount(); }
    virtual void setCapacity(int value) {}
    virtual int getCount() const = 0;
    bool isEmpty() const { return (getCount() <= 0); }
    char getDelimiter() const;
    void setDelimiter(char value);
    string getLineBreak() const;
    void setLineBreak(const char* value);
    char getQuoteChar() const;
    void setQuoteChar(char value);
    char getNameValueSeparator() const;
    void setNameValueSeparator(char value);
    string combineNameValue(const char* name, const char* value) const;
    string getName(int index) const;
    string getValue(const char* name) const;
    string getValue(int index) const;
    void setValue(const char* name, const char* value);
    void setValue(int index, const char* value);
    virtual POINTER getData(int index) const { return NULL; }
    virtual void setData(int index, POINTER data) {}
    virtual string getText() const;
    virtual void setText(const char* value);
    string getCommaText() const;
    void setCommaText(const char* value);
    string getDelimitedText() const;
    void setDelimitedText(const char* value);
    virtual const string& getString(int index) const = 0;
    virtual void setString(int index, const char* value);

    void beginUpdate();
    void endUpdate();

    Strings& operator = (const Strings& rhs);
    const string& operator[] (int index) const { return getString(index); }

protected:
    virtual void setUpdateState(bool isUpdating) {}
    virtual int compareStrings(const char* str1, const char* str2) const;

protected:
    void init();
    void error(const char* msg, int data) const;
    int getUpdateCount() const { return updateCount_; }
    string extractName(const char* str) const;

private:
    void assign(const Strings& src);

protected:
    UINT defined_;
    char delimiter_;
    string lineBreak_;
    char quoteChar_;
    char nameValueSeparator_;
    int updateCount_;
};

///////////////////////////////////////////////////////////////////////////////
// class StrList - 字符串列表类

class StrList : public Strings
{
public:
    friend int stringListCompareProc(const StrList& stringList, int index1, int index2);

public:
    /// The comparison function prototype that used by Sort().
    typedef int (*StringListCompareProc)(const StrList& stringList, int index1, int index2);

    /// Indicates the response when an application attempts to add a duplicate entry to a list.
    enum DUPLICATE_MODE
    {
        DM_IGNORE,      // Ignore attempts to add duplicate entries (do not add the duplicate).
        DM_ACCEPT,      // Allow the list to contain duplicate entries (add the duplicate).
        DM_ERROR        // Throw an exception when the application tries to add a duplicate.
    };

public:
    StrList();
    StrList(const StrList& src);
    virtual ~StrList();

    virtual int add(const char* str);
    virtual int add(const char* str, POINTER data);
    int add(const std::string& str) { return add(str.c_str()); }
    virtual void clear();
    virtual void del(int index);
    virtual void exchange(int index1, int index2);
    virtual int indexOf(const char* str) const;
    virtual void insert(int index, const char* str);
    virtual void insert(int index, const char* str, POINTER data);

    virtual int getCapacity() const { return capacity_; }
    virtual void setCapacity(int value);
    virtual int getCount() const { return count_; }
    virtual POINTER getData(int index) const;
    virtual void setData(int index, POINTER data);
    virtual const string& getString(int index) const;
    virtual void setString(int index, const char* value);

    virtual bool find(const char* str, int& index) const;
    virtual void sort();
    virtual void sort(StringListCompareProc compareProc);

    DUPLICATE_MODE getDupMode() const { return dupMode_; }
    void setDupMode(DUPLICATE_MODE value) { dupMode_ = value; }
    bool getSorted() const { return isSorted_; }
    virtual void setSorted(bool value);
    bool getCaseSensitive() const { return isCaseSensitive_; }
    virtual void setCaseSensitive(bool value);

    StrList& operator = (const StrList& rhs);

protected: // override
    virtual void setUpdateState(bool isUpdating);
    virtual int compareStrings(const char* str1, const char* str2) const;

protected: // virtual
    /// Occurs immediately before the list of strings changes.
    virtual void onChanging() {}
    /// Occurs immediately after the list of strings changes.
    virtual void onChanged() {}
    /// Internal method used to insert a string to the list.
    virtual void insertItem(int index, const char* str, POINTER data);

private:
    void init();
    void assign(const StrList& src);
    void internalClear();
    string& stringObjectNeeded(int index) const;
    void exchangeItems(int index1, int index2);
    void grow();
    void quickSort(int l, int r, StringListCompareProc compareProc);

private:
    struct StringItem
    {
        string *str;
        POINTER data;
    };

private:
    StringItem *list_;
    int count_;
    int capacity_;
    DUPLICATE_MODE dupMode_;
    bool isSorted_;
    bool isCaseSensitive_;
};

///////////////////////////////////////////////////////////////////////////////
// class Url - URL解析类

class Url
{
public:
    // The URL parts.
    enum URL_PART
    {
        URL_PROTOCOL  = 0x0001,
        URL_HOST      = 0x0002,
        URL_PORT      = 0x0004,
        URL_PATH      = 0x0008,
        URL_FILENAME  = 0x0010,
        URL_BOOKMARK  = 0x0020,
        URL_USERNAME  = 0x0040,
        URL_PASSWORD  = 0x0080,
        URL_PARAMS    = 0x0100,
        URL_ALL       = 0xFFFF,
    };

public:
    Url(const string& url = "");
    Url(const Url& src);
    virtual ~Url() {}

    void clear();
    Url& operator = (const Url& rhs);

    string getUrl() const;
    string getUrl(UINT parts);
    void setUrl(const string& value);

    const string& getProtocol() const { return protocol_; }
    const string& getHost() const { return host_; }
    const string& getPort() const { return port_; }
    const string& getPath() const { return path_; }
    const string& getFileName() const { return fileName_; }
    const string& getBookmark() const { return bookmark_; }
    const string& getUserName() const { return userName_; }
    const string& getPassword() const { return password_; }
    const string& getParams() const { return params_; }

    void setProtocol(const string& value) { protocol_ = value; }
    void setHost(const string& value) { host_ = value; }
    void setPort(const string& value) { port_ = value; }
    void setPath(const string& value) { path_ = value; }
    void setFileName(const string& value) { fileName_ = value; }
    void setBookmark(const string& value) { bookmark_ = value; }
    void setUserName(const string& value) { userName_ = value; }
    void setPassword(const string& value) { password_ = value; }
    void setParams(const string& value) { params_ = value; }

private:
    string protocol_;
    string host_;
    string port_;
    string path_;
    string fileName_;
    string bookmark_;
    string userName_;
    string password_;
    string params_;
};

///////////////////////////////////////////////////////////////////////////////
// class Packet - 数据包基类

class Packet
{
public:
    // 缺省内存增长步长 (字节数，必须是 2 的 N 次方)
    enum { DEFAULT_MEMORY_DELTA = 1024 };

public:
    Packet();
    virtual ~Packet();

    bool pack();
    bool unpack(void *buffer, int bytes);
    bool unpack(const Buffer& buffer);
    void clear();
    void ensurePacked();

    char* getBuffer() const { return (stream_? (char*)stream_->getMemory() : NULL); }
    int getSize() const { return (stream_? (int)stream_->getSize() : 0); }
    bool isAvailable() const { return isAvailable_; }
    bool IsPacked() const { return isPacked_; }

protected:
    void throwUnpackError();
    void throwPackError();
    void checkUnsafeSize(int value);

    void readBuffer(void *buffer, int bytes);
    void readINT8(INT8& value) { readBuffer(&value, sizeof(INT8)); }
    void readINT16(INT16& value) { readBuffer(&value, sizeof(INT16)); }
    void readINT32(INT32& value) { readBuffer(&value, sizeof(INT32)); }
    void readINT64(INT64& value) { readBuffer(&value, sizeof(INT64)); }
    void readString(std::string& str);
    void readBlob(std::string& str);
    void readBlob(Stream& stream);
    void readBlob(Buffer& buffer);
    INT8 readINT8() { INT8 v; readINT8(v); return v; }
    INT16 readINT16() { INT16 v; readINT16(v); return v; }
    INT32 readINT32() { INT32 v; readINT32(v); return v; }
    INT64 readINT64() { INT64 v; readINT64(v); return v; }
    bool readBool() { INT8 v; readINT8(v); return (v? true : false); }
    std::string readString() { std::string v; readString(v); return v; }

    void writeBuffer(const void *buffer, int bytes);
    void writeINT8(const INT8& value) { writeBuffer(&value, sizeof(INT8)); }
    void writeINT16(const INT16& value) { writeBuffer(&value, sizeof(INT16)); }
    void writeINT32(const INT32& value) { writeBuffer(&value, sizeof(INT32)); }
    void writeINT64(const INT64& value) { writeBuffer(&value, sizeof(INT64)); }
    void writeBool(bool value) { writeINT8(value ? 1 : 0); }
    void writeString(const std::string& str);
    void writeBlob(void *buffer, int bytes);
    void writeBlob(const Buffer& buffer);

    void fixStrLength(std::string& str, int length);
    void truncString(std::string& str, int maxLength);

protected:
    virtual void doPack() {}
    virtual void doUnpack() {}
    virtual void doAfterPack() {}
    virtual void doEncrypt() {}
    virtual void doDecrypt() {}
    virtual void doCompress() {}
    virtual void doDecompress() {}

private:
    void init();

protected:
    MemoryStream *stream_;
    bool isAvailable_;
    bool isPacked_;
};

///////////////////////////////////////////////////////////////////////////////
// class CustomObjectList - 对象列表基类

template<typename ObjectType>
class CustomObjectList
{
public:
    class Lock : public AutoInvokable
    {
    private:
        CriticalSection *lock_;
    protected:
        virtual void invokeInitialize() { if (lock_) lock_->lock(); }
        virtual void invokeFinalize() { if (lock_) lock_->unlock(); }
    public:
        Lock(bool active) : lock_(NULL) { if (active) lock_ = new CriticalSection(); }
        virtual ~Lock() { delete lock_; }
    };

    typedef ObjectType* ObjectPtr;

public:
    CustomObjectList() :
      lock_(false), isOwnsObjects_(true) {}

      CustomObjectList(bool isThreadSafe, bool isOwnsObjects) :
      lock_(isThreadSafe), isOwnsObjects_(isOwnsObjects) {}

      virtual ~CustomObjectList() { clear(); }

protected:
    virtual void notifyDelete(int index)
    {
        if (isOwnsObjects_)
        {
            ObjectPtr item = (ObjectPtr)items_[index];
            items_[index] = NULL;
            delete item;
        }
    }

protected:
    int add(ObjectPtr item, bool allowDuplicate = true)
    {
        ISE_ASSERT(item);
        AutoLocker locker(lock_);

        if (allowDuplicate || items_.indexOf(item) == -1)
            return items_.add(item);
        else
            return -1;
    }

    int remove(ObjectPtr item)
    {
        AutoLocker locker(lock_);

        int result = items_.indexOf(item);
        if (result >= 0)
        {
            notifyDelete(result);
            items_.del(result);
        }
        return result;
    }

    ObjectPtr extract(ObjectPtr item)
    {
        AutoLocker locker(lock_);

        ObjectPtr result = NULL;
        int i = items_.remove(item);
        if (i >= 0)
            result = item;
        return result;
    }

    ObjectPtr extract(int index)
    {
        AutoLocker locker(lock_);

        ObjectPtr result = NULL;
        if (index >= 0 && index < items_.getCount())
        {
            result = (ObjectPtr)items_[index];
            items_.del(index);
        }
        return result;
    }

    void del(int index)
    {
        AutoLocker locker(lock_);

        if (index >= 0 && index < items_.getCount())
        {
            notifyDelete(index);
            items_.del(index);
        }
    }

    void insert(int index, ObjectPtr item)
    {
        ISE_ASSERT(item);
        AutoLocker locker(lock_);
        items_.insert(index, item);
    }

    int indexOf(ObjectPtr item) const
    {
        AutoLocker locker(lock_);
        return items_.indexOf(item);
    }

    bool exists(ObjectPtr item) const
    {
        AutoLocker locker(lock_);
        return items_.indexOf(item) >= 0;
    }

    ObjectPtr first() const
    {
        AutoLocker locker(lock_);
        return (ObjectPtr)items_.first();
    }

    ObjectPtr last() const
    {
        AutoLocker locker(lock_);
        return (ObjectPtr)items_.last();
    }

    void clear()
    {
        AutoLocker locker(lock_);

        for (int i = items_.getCount() - 1; i >= 0; i--)
            notifyDelete(i);
        items_.clear();
    }

    void freeObjects()
    {
        AutoLocker locker(lock_);

        for (int i = items_.getCount() - 1; i >= 0; i--)
        {
            ObjectPtr item = (ObjectPtr)items_[i];
            items_[i] = NULL;
            delete item;
        }
    }

    int getCount() const { return items_.getCount(); }
    ObjectPtr& getItem(int index) const { return (ObjectPtr&)items_[index]; }
    CustomObjectList& operator = (const CustomObjectList& rhs) { items_ = rhs.items_; return *this; }
    ObjectPtr& operator[] (int index) const { return getItem(index); }
    bool isEmpty() const { return (getCount() <= 0); }
    void setOwnsObjects(bool value) { isOwnsObjects_ = value; }

protected:
    PointerList items_;       // 对象列表
    bool isOwnsObjects_;      // 元素被删除时，是否自动释放元素对象
    mutable Lock lock_;
};

///////////////////////////////////////////////////////////////////////////////
// class ObjectList - 对象列表类

template<typename ObjectType>
class ObjectList : public CustomObjectList<ObjectType>
{
public:
    ObjectList() :
        CustomObjectList<ObjectType>(false, true) {}
    ObjectList(bool isThreadSafe, bool isOwnsObjects) :
        CustomObjectList<ObjectType>(isThreadSafe, isOwnsObjects) {}
    virtual ~ObjectList() {}

    using CustomObjectList<ObjectType>::add;
    using CustomObjectList<ObjectType>::remove;
    using CustomObjectList<ObjectType>::extract;
    using CustomObjectList<ObjectType>::del;
    using CustomObjectList<ObjectType>::insert;
    using CustomObjectList<ObjectType>::indexOf;
    using CustomObjectList<ObjectType>::exists;
    using CustomObjectList<ObjectType>::first;
    using CustomObjectList<ObjectType>::last;
    using CustomObjectList<ObjectType>::clear;
    using CustomObjectList<ObjectType>::freeObjects;
    using CustomObjectList<ObjectType>::getCount;
    using CustomObjectList<ObjectType>::getItem;
    using CustomObjectList<ObjectType>::operator=;
    using CustomObjectList<ObjectType>::operator[];
    using CustomObjectList<ObjectType>::isEmpty;
    using CustomObjectList<ObjectType>::setOwnsObjects;
};

///////////////////////////////////////////////////////////////////////////////
// class CallbackList - 回调列表

template<typename T>
class CallbackList
{
public:
    void registerCallback(const T& callback)
    {
        AutoLocker locker(lock_);
        if (callback)
            items_.push_back(callback);
    }

    int getCount() const { return (int)items_.size(); }
    const T& getItem(int index) const { return items_[index]; }

private:
    std::vector<T> items_;
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////
// class ObjectContext - 从此类继承给对象添加上下文

class ObjectContext
{
public:
    void setContext(const boost::any& value) { context_ = value; }
    const boost::any& getContext() const { return context_; }
    boost::any& getContext() { return context_; }
private:
    boost::any context_;
};

///////////////////////////////////////////////////////////////////////////////
// class Logger - 日志类

class Logger : boost::noncopyable
{
public:
    ~Logger() {}
    static Logger& instance();

    void setFileName(const string& fileName, bool isNewFileDaily = false);

    void writeStr(const char *str);
    void writeStr(const string& str) { writeStr(str.c_str()); }
    void writeFmt(const char *format, ...);
    void writeException(const Exception& e);

private:
    Logger();

private:
    string getLogFileName();
    bool openFile(FileStream& fileStream, const string& fileName);
    void writeToFile(const string& str);

private:
    string fileName_;            // 日志文件名
    bool isNewFileDaily_;        // 是否每天用一个单独的文件存储日志
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////
// 全局函数

inline Logger& logger() { return Logger::instance(); }

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_CLASSES_H_
