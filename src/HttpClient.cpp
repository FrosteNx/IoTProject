#include "HttpClient.h"
#include <iostream>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "netdb.h"

const std::string HttpClient::USER_AGENT = "ESP32Client/1.0";

void HttpClient::sendGetRequest(const std::string& server_url) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res;
    int err = getaddrinfo(server_url.c_str(), "80", &hints, &res);
    if (err != 0 || res == nullptr) {
        std::cerr << "Failed to resolve server address, error code: " << err << std::endl;
        return;
    }

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        freeaddrinfo(res);
        return;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        std::cerr << "Connection to server failed" << std::endl;
        close(sock);
        freeaddrinfo(res);
        return;
    }

    freeaddrinfo(res);

    const std::string request = "GET / HTTP/1.1\r\nHost: " + server_url + "\r\nUser-Agent: " + USER_AGENT + "\r\nConnection: close\r\n\r\n";
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
        std::cerr << "Sending GET request failed" << std::endl;
        close(sock);
        return;
    }

    char buffer[1024];
    int received;
    while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        std::cout << buffer;
    }

    if (received < 0) {
        std::cerr << "Receiving data failed" << std::endl;
    } else {
        std::cout << "\nResponse received successfully." << std::endl;
    }

    close(sock);
}