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
    string getInfo1(const PropertyList& args, string& contentType);
    string getInfo2(const PropertyList& args, string& contentType);
    string getInfo3(const PropertyList& args, string& contentType);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_INSPECTOR_H_
