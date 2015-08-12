#include "CleverbotHTTP.h"

#include <sstream>
#include "MD5.h"

std::size_t CurlWriteString(void* buffer, std::size_t size, std::size_t nmemb, void* result) {
    if (result) {
        std::string* out = static_cast<std::string*>(result);
        out->append(static_cast<char*>(buffer), nmemb);
    }

    return size * nmemb;
}

CleverbotHTTP::CleverbotHTTP(long timeout)
    : m_Header({
        "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36",
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
        "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7",
        "Accept-Language: en-us,en;q=0.8,en-us;q=0.5,en;q=0.3",
        "Cache-Control: no-cache",
        "Host: " + m_Host,
        "Origin: " + m_Protocol + m_Host + "/",
        "Referer: " + m_Protocol + m_Host + "/",
        "Pragma: no-cache"
      }),
      m_Data({
        { "stimulus", "" },
        { "vText2", "" }, // Newest convo message
        { "vText3", "" },
        { "vText4", "" },
        { "vText5", "" },
        { "vText6", "" },
        { "vText7", "" },
        { "vText8", "" }, // Oldest convo message
        { "sessionid", "" },
        { "cb_settings_language", "en" },
        { "cb_settings_scripting", "no" },
        { "islearning", "1" },
        { "icognoid", "wsf" },
        { "icognocheck", "" }
      }),
      m_Timeout(timeout)
{
    m_Curl = curl_easy_init();

    InitializeCookies();
}

CleverbotHTTP::~CleverbotHTTP() {
    curl_easy_cleanup(m_Curl);
}

std::string CleverbotHTTP::URLEncode(const std::string& str) const {
    std::string result;

    char* escaped = curl_easy_escape(m_Curl, str.c_str(), str.length());
    result = escaped;
    curl_free(escaped);

    return result;
}

// Initializes XVIS cookie
bool CleverbotHTTP::InitializeCookies() {
    CURLcode res;

    curl_slist* header = CreateHeader();

    curl_easy_setopt(m_Curl, CURLOPT_URL, (m_Protocol + m_Host).c_str());
    curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, CurlWriteString);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT_MS, m_Timeout);

    res = curl_easy_perform(m_Curl);

    curl_slist_free_all(header);

    return res == CURLE_OK;
}

std::string CleverbotHTTP::CreateCookie(const std::string& key, const std::string& value) {
    std::stringstream cookie;

    cookie << m_Host << "\t" << "FALSE" << "\t" << "/" << "\t" << "FALSE" << "\t" << "0" << "\t";
    cookie << key << "\t" << value;

    return cookie.str();
}

std::string CleverbotHTTP::Think(const std::string& thought) {
    if (thought.length() == 0) return "";

    m_Data[Stimulus].second = URLEncode(thought);

    std::string result = Send();

    return result;
}

std::string CleverbotHTTP::Send() {
    CURLcode res;
    if (!m_Curl) return "";

    UpdateConversation();

    m_Data[IcognoCheck].second = GetDigest();

    curl_slist* header = CreateHeader();
    std::string post = CreatePOST();
    std::string data;

    curl_easy_setopt(m_Curl, CURLOPT_URL, m_APIUrl.c_str());
    curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post.c_str());
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDSIZE, post.length());
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, CurlWriteString);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT_MS, m_Timeout);

    res = curl_easy_perform(m_Curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        data = "";
    }

    curl_slist_free_all(header);

    if (!data.empty()) {
        std::pair<std::string, std::string> parsed = ParseResult(data);

        std::string answer = parsed.first;
        std::string sessionid = parsed.second;

        m_Conversation.push_back(m_Data[Stimulus].second);
        m_Conversation.push_back(URLEncode(answer));

        m_Data[SessionID].second = sessionid;

        return answer;
    }

    return "";
}

void CleverbotHTTP::UpdateConversation() {
    using namespace std;

    for (size_t i = m_Conversation.size(); i > 0; --i) {
        string line = m_Conversation[i - 1];
        size_t current = m_Conversation.size() - i;

        if (current > 6) break;

        m_Data[VText2 + current].second = line;
    }
}

std::string CleverbotHTTP::GetDigest() const {
    using namespace std;

    stringstream ss;

    ss << "stimulus=" << m_Data[Stimulus].second;
    for (size_t i = 1; i < m_Data.size(); ++i) {
        if (m_Data[i].second.length() > 0)
            ss << "&" << m_Data[i].first << "=" << m_Data[i].second;
    }

    string token = md5(ss.str().substr(9, 35 - 9));

    return URLEncode(token);
}

curl_slist* CleverbotHTTP::CreateHeader() const {
    curl_slist* list = nullptr;

    for (const std::string& value : m_Header)
        list = curl_slist_append(list, value.c_str());

    return list;
}

std::string CleverbotHTTP::CreatePOST() const {
    std::stringstream ss;

    ss << m_Data[Stimulus].first << "=" << m_Data[Stimulus].second;

    for (std::size_t i = 1; i < m_Data.size(); ++i) {
        const DataPair& kv = m_Data.at(i);

        if (kv.second.length() == 0) continue;

        ss << "&" << kv.first << "=" << kv.second;
    }

    return ss.str();
}

std::pair<std::string, std::string> CleverbotHTTP::ParseResult(std::string result) {
    using namespace std;

    std::pair<std::string, std::string> parsed = { "", "" };

    for (int i = 0; i < 2; ++i) {
        size_t pos = result.find("\r");

        if (pos == string::npos) return parsed;

        std::string current = result.substr(0, pos);
        result = result.substr(pos + 1);

        if (i == 0)
            parsed.first = current;
        else
            parsed.second = current;
    }

    return parsed;
}
