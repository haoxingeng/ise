///////////////////////////////////////////////////////////////////////////////

#include "app_business.h"
#include "svr_mod_echo.h"
#include "svr_mod_discard.h"
#include "svr_mod_daytime.h"
#include "svr_mod_chargen.h"

//-----------------------------------------------------------------------------

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    IseSvrModBusiness::initialize();
}

//-----------------------------------------------------------------------------

void AppBusiness::finalize()
{
    IseSvrModBusiness::finalize();

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
    options.setLogFileName(getAppSubPath("log") + changeFileExt(extractFileName(getAppExeName()), ".log"), true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);
    options.setServerType(ST_TCP);
}

//-----------------------------------------------------------------------------

void AppBusiness::createServerModules(IseServerModuleList& svrModList)
{
    svrModList.push_back(new ServerModule_Echo());
    svrModList.push_back(new ServerModule_Discard());
    svrModList.push_back(new ServerModule_Daytime());
    svrModList.push_back(new ServerModule_Chargen());
}
