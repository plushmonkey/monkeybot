#ifndef CLEVERBOTHTTP_H_
#define CLEVERBOTHTTP_H_

#include <string>
#include <vector>

#include "curl/curl.h"

class CleverbotHTTP {
private:
    const std::string m_Protocol = "http://";
    const std::string m_Host = "www.cleverbot.com";
    const std::string m_Resource = "/webservicemin";
    const std::string m_APIUrl = m_Protocol + m_Host + m_Resource;
    const std::vector<std::string> m_Header;
    const long m_Timeout;

    enum DataType { Stimulus, VText2, VText3, VText4, VText5, VText6, VText7, VText8, SessionID, CBSettingsLanguage, CBSettingsScripting, IsLearning, IcognoID, IcognoCheck };
    typedef std::pair<std::string, std::string> DataPair;

    CURL* m_Curl = nullptr;

    std::vector<DataPair> m_Data;
    std::vector<std::string> m_Conversation;

    std::string URLEncode(const std::string& str) const;
    std::string Send();
    std::pair<std::string, std::string> ParseResult(std::string result);
    curl_slist* CreateHeader() const;
    std::string CreatePOST() const;
    std::string GetDigest() const;
    void UpdateConversation();
    bool InitializeCookies();
    std::string CreateCookie(const std::string& key, const std::string& value);

public:
    CleverbotHTTP(long timeout = 8000);
    ~CleverbotHTTP();

    std::string Think(const std::string& thought);
};

#endif
