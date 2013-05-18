///////////////////////////////////////////////////////////////////////////////

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseBusiness
{
public:
    AppBusiness() {}
    virtual ~AppBusiness() {}

    virtual void initialize();
    virtual void finalize();
    virtual void initIseOptions(IseOptions& options);

private:
    void threadPoolTask(Thread& thread);

private:
    ThreadPool threadPool_;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _THREAD_POOL_H_
