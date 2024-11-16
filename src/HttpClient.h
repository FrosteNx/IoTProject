#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>

class HttpClient {
public:
    void sendGetRequest(const std::string& server_url);

private:
    static const std::string USER_AGENT;
};

#endif // HTTP_CLIENT_H