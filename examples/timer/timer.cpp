///////////////////////////////////////////////////////////////////////////////

#include "timer.h"

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    everyTimerId_ = 0;
    everyTimerExpiredCount_ = 0;
}

//-----------------------------------------------------------------------------

void AppBusiness::finalize()
{
    string msg = formatString("%s stoped.", getAppExeName(false).c_str());
    std::cout << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::afterInit()
{
    string msg = formatString("%s started.", getAppExeName(false).c_str());
    std::cout << std::endl << msg << std::endl;
    logger().writeStr(msg);

    // after 3 seconds
    iseApp().timerManager().executeAt(Timestamp::now() + 3 * MILLISECS_PER_SECOND,
        boost::bind(&AppBusiness::onTimerAt, this));

    // after 5 seconds
    iseApp().timerManager().executeAfter(5 * MILLISECS_PER_SECOND,
        boost::bind(&AppBusiness::onTimerAfter, this));

    // every 1 second
    everyTimerId_ = iseApp().timerManager().executeEvery(1 * MILLISECS_PER_SECOND,
        boost::bind(&AppBusiness::onTimerEvery, this));
}

//-----------------------------------------------------------------------------

void AppBusiness::onInitFailed(Exception& e)
{
    string msg = formatString("fail to start %s.", getAppExeName(false).c_str());
    std::cout << std::endl << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setServerType(0);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTimerAt()
{
    logger().writeStr("onTimerAt called.");
}

//-----------------------------------------------------------------------------

void AppBusiness::onTimerAfter()
{
    logger().writeStr("onTimerAfter called.");
}

//-----------------------------------------------------------------------------

void AppBusiness::onTimerEvery()
{
    logger().writeFmt("onTimerEvery called. (%d)", everyTimerExpiredCount_);

    everyTimerExpiredCount_++;
    if (everyTimerExpiredCount_ >= 10)
        iseApp().timerManager().cancelTimer(everyTimerId_);
}
