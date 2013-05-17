///////////////////////////////////////////////////////////////////////////////

#ifndef _SVR_MOD_INSPECTOR_H_
#define _SVR_MOD_INSPECTOR_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class ServerModule_Inspector: public IseServerInspector
{
public:
    ServerModule_Inspector();
    virtual ~ServerModule_Inspector() {}
private:
    std::string getInfo1(const PropertyList& args, std::string& contentType);
    std::string getInfo2(const PropertyList& args, std::string& contentType);
    std::string getInfo3(const PropertyList& args, std::string& contentType);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_INSPECTOR_H_
