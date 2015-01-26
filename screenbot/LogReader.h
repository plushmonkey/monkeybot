#ifndef LOG_READER_H_
#define LOG_READER_H_

#include "Common.h"
#include "FileStream.h"

class LogReader {
private:
    FileStream m_FileStream;
    DWORD m_UpdateTimer;
    unsigned int m_UpdateFrequency;
    
public:
    LogReader(const std::string& filename, const unsigned int frequency);

    void Clear();
    void Update(unsigned long dt);
};

#endif
