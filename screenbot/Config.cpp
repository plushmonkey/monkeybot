#include "Config.h"
#include <regex>
#include <fstream>
#include <tchar.h>
#include <algorithm>

const TCHAR regex_str[] = _T("^\\s*([a-zA-Z_]\\w*)\\s*=\\s*(.+?)\\s*$");

namespace Convert {
    template<>
    int Get(const tstring& val) {
        if (val.length() == 0) return 0;
        return _tstoi(val.c_str());
    }

    template<>
    unsigned int Get(const tstring& val) {
        if (val.length() == 0) return 0;
        return _tstoi(val.c_str());
    }

    template<>
    tstring Get(const tstring& val) {
        return val;
    }

    template<>
    bool Get(const tstring& val) {
        if (val.length() == 0) return false;

        tstring out;
        out.resize(val.length());

        std::transform(val.begin(), val.end(), out.begin(), _totlower);

        return out.compare(_T("true")) == 0 || out.compare(_T("t")) == 0;
    }

    template<>
    TCHAR Get(const tstring& val) {
        if (val.length() == 0) return '\0';
        return val.at(0);
    }
}

void Config::Set(const tstring& var, const tstring& val) {
    tstring out;
    out.resize(var.length());
    std::transform(var.begin(), var.end(), out.begin(), _totlower);
    m_Variables[out] = val;
}

bool Config::Load(const tstring& filename) {
    tifstream file;
    file.open(filename, std::ios::in);

    tregex regex(regex_str);

    if (!file.is_open())
        return false;

    tstring data;

    file.seekg(0, std::ios::end);
    data.reserve(static_cast<unsigned int>(file.tellg()));
    file.seekg(0, std::ios::beg);

    data.assign((std::istreambuf_iterator<TCHAR>(file)), std::istreambuf_iterator<TCHAR>());

    tsregex_iterator begin(data.begin(), data.end(), regex);
    tsregex_iterator end;

    for (tsregex_iterator iter = begin; iter != end; ++iter) {
        tsmatch match = *iter;

        Set(match[1], match[2]);
    }

    return true;
}
