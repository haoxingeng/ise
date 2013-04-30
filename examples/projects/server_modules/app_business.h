///////////////////////////////////////////////////////////////////////////////

#ifndef _APP_BUSINESS_H_
#define _APP_BUSINESS_H_

#include "ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseSvrModBusiness
{
public:
    AppBusiness() {}
    virtual ~AppBusiness() {}

    virtual void initialize();
    virtual void finalize();

    virtual void doStartupState(STARTUP_STATE state);
    virtual void initIseOptions(IseOptions& options);

protected:
    virtual void createServerModules(PointerList& svrModList);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _APP_BUSINESS_H_
