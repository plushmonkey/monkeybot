#ifndef API_VERSION_H_
#define API_VERSION_H_

#include <string>

namespace api {

class Version {
public:
    /**
    * \brief Gets the major version number.
    */
    virtual int GetMajor() = 0;
    /**
    * \brief Gets the minor version number.
    */
    virtual int GetMinor() = 0;
    /**
    * \brief Gets the revision number.
    */
    virtual int GetRevision() = 0;
    /**
    * \brief Gets the version string.
    */
    virtual std::string GetString() = 0;

    virtual ~Version() { }
};

} // ns
#endif
