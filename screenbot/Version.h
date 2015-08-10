#ifndef VERSION_H_
#define VERSION_H_

#include "api/Version.h"

class Version : public api::Version {
public:
    int GetMajor();
    int GetMinor();
    int GetRevision();
    std::string GetString();
};

#endif
