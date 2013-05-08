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

    const char *msg = "4in1-server stoped.";
    std::cout << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::doStartupState(STARTUP_STATE state)
{
    switch (state)
    {
    case SS_AFTER_START:
        {
            const char *msg = "4in1-server started.";
            std::cout << std::endl << msg << std::endl;
            logger().writeStr(msg);
        }
        break;

    case SS_START_FAIL:
        {
            const char *msg = "Fail to start 4in1-server.";
            std::cout << std::endl << msg << std::endl;
            logger().writeStr(msg);
        }
        break;

    default:
        break;
    }
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setLogFileName(getAppSubPath("log") + "4in1-server-log.txt", true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);
    options.setServerType(ST_TCP);
    options.setTcpEventLoopCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::createServerModules(PointerList& svrModList)
{
	svrModList.add(new ServerModule_Echo());
	svrModList.add(new ServerModule_Discard());
	svrModList.add(new ServerModule_Daytime());
	svrModList.add(new ServerModule_Chargen());
}
