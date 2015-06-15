#include "Version.h"

#include <sstream>

namespace {

const int Major = 4;
const int Minor = 0;
const int Revision = 1;

} // ns

int Version::GetMajor() {
    return Major;
}

int Version::GetMinor() {
    return Minor;
}

int Version::GetRevision() {
    return Revision;
}

std::string Version::GetString() {
    std::stringstream ss;

    ss << GetMajor() << "." << GetMinor() << "." << GetRevision();
    return ss.str();
}
