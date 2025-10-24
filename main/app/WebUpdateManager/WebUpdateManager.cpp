#include "WebUpdateManager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <vector>
#include <sys/stat.h>
#include "esp_crt_bundle.h"

// --- Tiny streaming TAR extractor ---
class TarExtractor
{
public:
    TarExtractor(const char *rootPath) : root(rootPath) {}
    ~TarExtractor()
    {
        if (current)
            fclose(current);
    }

    void feed(const uint8_t *data, size_t len)
    {
        buffer.insert(buffer.end(), data, data + len);
        process();
    }

    void finish() { process(true); }

private:
    std::string root;
    std::vector<uint8_t> buffer;
    FILE *current = nullptr;
    size_t remaining = 0;
    size_t padding = 0;

    struct TarHeader
    {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag;
        char linkname[100];
    };

    void process(bool final = false)
    {
        while (true)
        {
            // Write remaining data for current file
            if (remaining > 0 && buffer.size() > 0)
            {
                size_t chunk = std::min(buffer.size(), remaining);
                fwrite(buffer.data(), 1, chunk, current);
                buffer.erase(buffer.begin(), buffer.begin() + chunk);
                remaining -= chunk;
                if (remaining == 0)
                {
                    if (padding)
                    {
                        size_t skip = std::min(buffer.size(), padding);
                        buffer.erase(buffer.begin(), buffer.begin() + skip);
                        padding -= skip;
                    }
                    fclose(current);
                    current = nullptr;
                }
                continue;
            }

            if (buffer.size() < 512)
                break;

            TarHeader *hdr = (TarHeader *)buffer.data();
            if (hdr->name[0] == '\0')
            {
                buffer.clear();
                break;
            }

            size_t filesize = strtol(hdr->size, nullptr, 8);
            std::string path = root + "/" + std::string(hdr->name);
            ESP_LOGI("TAR", "Extracting %s (%d bytes)", path.c_str(), (int)filesize);

            // Create directories if needed
            auto pos = path.find_last_of('/');
            if (pos != std::string::npos)
            {
                std::string dir = path.substr(0, pos);
                mkdir(dir.c_str(), 0777);
            }

            current = fopen(path.c_str(), "wb");
            buffer.erase(buffer.begin(), buffer.begin() + 512);
            size_t chunk = std::min(buffer.size(), filesize);
            if (chunk > 0)
            {
                fwrite(buffer.data(), 1, chunk, current);
                buffer.erase(buffer.begin(), buffer.begin() + chunk);
            }
            remaining = filesize - chunk;
            padding = ((filesize + 511) & ~511) - filesize;
        }
    }
};


#define MANIFEST_URL_MAX 256
#define BUNDLE_URL_MAX   256

struct WebUpdateUrls
{
    char manifestUrl[MANIFEST_URL_MAX];
    char bundleUrl[BUNDLE_URL_MAX];
    bool success;
};

WebUpdateUrls FetchReleaseUrls()
{
    const char *TAG = "WebUpdateManager";
    WebUpdateUrls result = {};
    result.success = false;

    ESP_LOGI(TAG, "Fetching latest release info from GitHub...");

    static char response_buffer[2048]; // Enough for JSON
    esp_http_client_config_t config = {};
        config.url = "https://api.github.com/repos/vanBassum/Thermy-ui/releases/latest";
        config.transport_type = HTTP_TRANSPORT_OVER_SSL;
        config.crt_bundle_attach = esp_crt_bundle_attach;
        config.timeout_ms = 20000;
        config.buffer_size_tx = 2048;
        config.buffer_size = 2048;
        config.user_data = response_buffer;
    

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "User-Agent", "ESP32-WebUpdater");

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return result;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200)
    {
        ESP_LOGE(TAG, "HTTP status %d", status_code);
        esp_http_client_cleanup(client);
        return result;
    }

    int content_length = esp_http_client_get_content_length(client);
    int read_len = esp_http_client_read_response(client, response_buffer, sizeof(response_buffer) - 1);
    esp_http_client_cleanup(client);

    if (read_len <= 0)
    {
        ESP_LOGE(TAG, "Failed to read response");
        return result;
    }

    response_buffer[read_len] = '\0';
    ESP_LOGI(TAG, "Got release JSON (%d bytes)", read_len);

    // Parse URLs
    const char *manifestKey = "\"name\":\"manifest.json\"";
    const char *bundleKey = "\"name\":\"webbundle.tar\"";
    const char *manifestPos = strstr(response_buffer, manifestKey);
    const char *bundlePos = strstr(response_buffer, bundleKey);

    if (manifestPos)
    {
        const char *start = strstr(manifestPos, "https://");
        const char *end = strchr(start, '"');
        if (start && end)
        {
            size_t len = end - start;
            if (len >= sizeof(result.manifestUrl)) len = sizeof(result.manifestUrl) - 1;
            memcpy(result.manifestUrl, start, len);
            result.manifestUrl[len] = '\0';
        }
    }

    if (bundlePos)
    {
        const char *start = strstr(bundlePos, "https://");
        const char *end = strchr(start, '"');
        if (start && end)
        {
            size_t len = end - start;
            if (len >= sizeof(result.bundleUrl)) len = sizeof(result.bundleUrl) - 1;
            memcpy(result.bundleUrl, start, len);
            result.bundleUrl[len] = '\0';
        }
    }

    result.success = result.manifestUrl[0] && result.bundleUrl[0];
    return result;
}

// --- Manager Implementation ---
WebUpdateManager::WebUpdateManager(ServiceProvider &ctx)
    : settingsManager(ctx.GetSettingsManager()) {}

void WebUpdateManager::Init()
{
    if (initGuard.IsReady())
        return;

    ESP_LOGI(TAG, "WebUpdateManager initialized");
    initGuard.SetReady();
}

void WebUpdateManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (!ctx.HasElapsed(lastCheck, CHECK_INTERVAL))
        return;

    ESP_LOGI(TAG, "Checking for web bundle update...");

    WebUpdateUrls urls = FetchReleaseUrls();
    if (urls.success)
    {
        ESP_LOGI("Updater", "Manifest: %s", urls.manifestUrl);
        ESP_LOGI("Updater", "Bundle: %s", urls.bundleUrl);
        // You can now call FetchManifest(urls.manifestUrl, ...)
    }
    else
    {
        ESP_LOGE("Updater", "Failed to retrieve release URLs");
    }



    std::string remoteJson;
    if (!FetchManifest(remoteJson))
        return;

    ESP_LOGI(TAG, "Fetched remote manifest");

    std::string remoteHash;
    if (!ParseManifest(remoteJson, remoteHash))
        return;

    ESP_LOGI(TAG, "Remote web bundle hash: %s", remoteHash.c_str());

    std::string localHash;
    ReadLocalHash(localHash);

    ESP_LOGI(TAG, "Local web bundle hash: %s", localHash.empty() ? "<none>" : localHash.c_str());

    if (HashChanged(localHash, remoteHash))
    {
        ESP_LOGI(TAG, "New web bundle detected, updating...");
        if (DownloadAndExtractBundle())
        {
            WriteLocalManifest(remoteJson);
            ESP_LOGI(TAG, "Web bundle updated successfully");
        }
        else
        {
            ESP_LOGW(TAG, "Failed to update web bundle");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Web bundle up to date");
    }
    ctx.MarkExecuted(lastCheck, CHECK_INTERVAL);
}

bool WebUpdateManager::FetchManifest(std::string &jsonOut)
{
    const char *TAG = "WebUpdateManager";

    ESP_LOGI(TAG, "Fetching manifest from GitHub...");

    esp_http_client_config_t config = {
        .url = "https://github.com/vanBassum/Thermy-ui/releases/download/latest/manifest.json",
        .timeout_ms = 10000,
        .disable_auto_redirect = false,
        .crt_bundle_attach = esp_crt_bundle_attach, // ✅ HTTPS verification
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    //ESP_LOGI(TAG, "Connected to: %s", esp_http_client_get_url(client));
    int status_code = esp_http_client_get_status_code(client);
    int64_t content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP status: %d, content length: %lld", status_code, content_length);

    char buf[512];
    int total = 0;

    while (true)
    {
        int len = esp_http_client_read(client, buf, sizeof(buf));
        if (len < 0)
        {
            ESP_LOGE(TAG, "HTTP read error: %d", len);
            break;
        }
        if (len == 0)
        {
            ESP_LOGI(TAG, "HTTP read complete");
            break;
        }

        jsonOut.append(buf, len);
        total += len;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total > 0)
    {
        ESP_LOGI(TAG, "Successfully fetched manifest (%d bytes)", total);
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Manifest fetch failed or empty response");
        return false;
    }
}

bool WebUpdateManager::ParseManifest(const std::string &json, std::string &remoteHash)
{
    cJSON *root = cJSON_Parse(json.c_str());
    if (!root)
        return false;
    cJSON *sha = cJSON_GetObjectItem(root, "sha256");
    if (sha && cJSON_IsString(sha))
        remoteHash = sha->valuestring;
    cJSON_Delete(root);
    return !remoteHash.empty();
}

bool WebUpdateManager::ReadLocalHash(std::string &hashOut)
{
    FILE *f = fopen("/fat/manifest.json", "r");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    std::string content(size, 0);
    fread(content.data(), 1, size, f);
    fclose(f);
    return ParseManifest(content, hashOut);
}

bool WebUpdateManager::WriteLocalManifest(const std::string &json)
{
    FILE *f = fopen("/fat/manifest.json", "w");
    if (!f)
        return false;
    fwrite(json.data(), 1, json.size(), f);
    fclose(f);
    return true;
}

bool WebUpdateManager::HashChanged(const std::string &local, const std::string &remote)
{
    return local != remote;
}

bool WebUpdateManager::DownloadAndExtractBundle()
{
    esp_http_client_config_t cfg = {
        .url = "https://raw.githubusercontent.com/vanBassum/Thermy-ui/main/webbundle.tar",
        .timeout_ms = 60000,
        .disable_auto_redirect = false,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client)
        return false;

    if (esp_http_client_open(client, 0) != ESP_OK)
    {
        esp_http_client_cleanup(client);
        return false;
    }

    uint8_t buf[1024];
    TarExtractor tar("/fat");

    bool ok = true;
    while (true)
    {
        int n = esp_http_client_read(client, (char *)buf, sizeof(buf));
        if (n <= 0)
            break;
        tar.feed(buf, n);
    }

    tar.finish();
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return ok;
}
