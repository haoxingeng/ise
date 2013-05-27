///////////////////////////////////////////////////////////////////////////////

#include "thread_pool.h"

//-----------------------------------------------------------------------------

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    threadPool_.addTask(boost::bind(&AppBusiness::threadPoolTask, this, _1));
    threadPool_.setTaskRepeat(true);
    threadPool_.start(10);
}

//-----------------------------------------------------------------------------

void AppBusiness::finalize()
{
    threadPool_.stop();
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setIsDaemon(false);
}

//-----------------------------------------------------------------------------

void AppBusiness::threadPoolTask(Thread& thread)
{
    static AtomicInt counter;

    counter.increment();
    string s = formatString("executing task: %u. (thread: %u)", counter.get(), thread.getThreadId());
    logger().writeStr(s);
    std::cout << s << std::endl;

    thread.sleep(1);
}
