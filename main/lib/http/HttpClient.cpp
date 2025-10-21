#include "HttpClient.h"


HttpClient::HttpClient() {}
HttpClient::~HttpClient() {}

HttpRequest HttpClient::createRequest(const char* url, esp_http_client_method_t method, TickType_t timeout)
{
    return HttpRequest(url, method, timeout);
}
