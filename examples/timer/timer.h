///////////////////////////////////////////////////////////////////////////////

#ifndef _TIMER_H_
#define _TIMER_H_

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

    virtual void afterInit();
    virtual void onInitFailed(Exception& e);
    virtual void initIseOptions(IseOptions& options);

private:
    void onTimerAt();
    void onTimerAfter();
    void onTimerEvery();
};

///////////////////////////////////////////////////////////////////////////////

#endif // _TIMER_H_
