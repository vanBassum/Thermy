#include "HttpWebServer.h"

HttpWebServer::HttpWebServer()
{
}

HttpWebServer::~HttpWebServer()
{
    stop();
}

void HttpWebServer::start()
{
    if (server)
        return;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 8;  // allow 8 concurrent sockets
    config.max_uri_handlers = 16; // more endpoints

    config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_ERROR_CHECK(httpd_start(&server, &config));
}

void HttpWebServer::stop()
{
    if (!server)
        return;
    httpd_stop(server);
    server = nullptr;
}

void HttpWebServer::EnableCors()
{
    // Handle OPTIONS for CORS preflight
    httpd_uri_t api_options = {
        .uri = "/*",
        .method = HTTP_OPTIONS,
        .handler = api_options_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &api_options);
}

void HttpWebServer::registerHandler(const char *uri, httpd_method_t method, HttpServerEndpoint &handler)
{
    httpd_uri_t config = {
        .uri = uri,
        .method = method,
        .handler = &HttpWebServer::trampoline,
        .user_ctx = &handler};
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config));
}

void HttpWebServer::set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

esp_err_t HttpWebServer::trampoline(httpd_req_t *req)
{
    set_cors_headers(req); // Always inject CORS headers
    auto *h = static_cast<HttpServerEndpoint *>(req->user_ctx);
    return h->handle(req);
}

esp_err_t HttpWebServer::api_options_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}
