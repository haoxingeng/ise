///////////////////////////////////////////////////////////////////////////////

#ifndef _APP_BUSINESS_H_
#define _APP_BUSINESS_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseSvrModBusiness
{
public:
    AppBusiness() {}
    virtual ~AppBusiness() {}

    virtual void initialize();
    virtual void finalize();

    virtual void afterInit();
    virtual void onInitFailed(Exception& e);
    virtual void initIseOptions(IseOptions& options);

protected:
    virtual void createServerModules(IseServerModuleList& svrModList);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _APP_BUSINESS_H_
