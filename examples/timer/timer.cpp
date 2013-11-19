///////////////////////////////////////////////////////////////////////////////

#include "timer.h"

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    // nothing
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
    iseApp().timerManager().executeAt(Timestamp::now() + MICROSECS_PER_SECOND * 3,
        boost::bind(&AppBusiness::onTimerAt, this));

    // after 5 seconds
    iseApp().timerManager().executeAfter(5, boost::bind(&AppBusiness::onTimerAfter, this));

    // every 1 second
    iseApp().timerManager().executeEvery(1, boost::bind(&AppBusiness::onTimerEvery, this));
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
    logger().writeStr("onTimerAt");
}

//-----------------------------------------------------------------------------

void AppBusiness::onTimerAfter()
{
    logger().writeStr("onTimerAfter");
}

//-----------------------------------------------------------------------------

void AppBusiness::onTimerEvery()
{
    logger().writeStr("onTimerEvery");
}
