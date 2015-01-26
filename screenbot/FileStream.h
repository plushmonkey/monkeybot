#ifndef FILE_STREAM_H_
#define FILE_STREAM_H_

#include <string>
#include <fstream>
#include <iostream>

class FileStream {
private:
    std::string mFilename;
    std::streamoff mPrevSize;

    FileStream(const FileStream& other);
    FileStream& operator=(const FileStream& other);

public:
    FileStream(const std::string& filename)
        : mFilename(filename), mPrevSize(0) {  }

    FileStream& operator>>(std::string& out) {
        std::ifstream in(mFilename, std::ios::in);

        out.assign("");

        if (!in.is_open()) return *this;

        in.seekg(0, std::ios::end);

        int len = static_cast<unsigned int>(in.tellg());
        in.seekg(mPrevSize, std::ios::beg);

        int diff = len - static_cast<unsigned int>(mPrevSize);

        if (diff <= 0) {
            in.close();
            return *this;
        }

        out.resize(diff);

        in.read(const_cast<char *>(out.c_str()), diff);

        in.close();

        mPrevSize = len;

        return *this;
    }
};

#endif
