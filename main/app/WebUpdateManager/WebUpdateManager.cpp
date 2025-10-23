#include "WebUpdateManager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <vector>
#include <sys/stat.h>
#include "esp_crt_bundle.h"


// --- Tiny streaming TAR extractor ---
class TarExtractor {
public:
    TarExtractor(const char* rootPath) : root(rootPath) {}
    ~TarExtractor() { if (current) fclose(current); }

    void feed(const uint8_t* data, size_t len) {
        buffer.insert(buffer.end(), data, data + len);
        process();
    }

    void finish() { process(true); }

private:
    std::string root;
    std::vector<uint8_t> buffer;
    FILE* current = nullptr;
    size_t remaining = 0;
    size_t padding = 0;

    struct TarHeader {
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

    void process(bool final = false) {
        while (true) {
            // Write remaining data for current file
            if (remaining > 0 && buffer.size() > 0) {
                size_t chunk = std::min(buffer.size(), remaining);
                fwrite(buffer.data(), 1, chunk, current);
                buffer.erase(buffer.begin(), buffer.begin() + chunk);
                remaining -= chunk;
                if (remaining == 0) {
                    if (padding) {
                        size_t skip = std::min(buffer.size(), padding);
                        buffer.erase(buffer.begin(), buffer.begin() + skip);
                        padding -= skip;
                    }
                    fclose(current);
                    current = nullptr;
                }
                continue;
            }

            if (buffer.size() < 512) break;

            TarHeader* hdr = (TarHeader*)buffer.data();
            if (hdr->name[0] == '\0') {
                buffer.clear();
                break;
            }

            size_t filesize = strtol(hdr->size, nullptr, 8);
            std::string path = root + "/" + std::string(hdr->name);
            ESP_LOGI("TAR", "Extracting %s (%d bytes)", path.c_str(), (int)filesize);

            // Create directories if needed
            auto pos = path.find_last_of('/');
            if (pos != std::string::npos) {
                std::string dir = path.substr(0, pos);
                mkdir(dir.c_str(), 0777);
            }

            current = fopen(path.c_str(), "wb");
            buffer.erase(buffer.begin(), buffer.begin() + 512);
            size_t chunk = std::min(buffer.size(), filesize);
            if (chunk > 0) {
                fwrite(buffer.data(), 1, chunk, current);
                buffer.erase(buffer.begin(), buffer.begin() + chunk);
            }
            remaining = filesize - chunk;
            padding = ((filesize + 511) & ~511) - filesize;
        }
    }
};

// --- Manager Implementation ---
WebUpdateManager::WebUpdateManager(ServiceProvider &ctx)
    : settingsManager(ctx.GetSettingsManager()) {}

void WebUpdateManager::Init() {
    if (initGuard.IsReady())
        return;

    ESP_LOGI(TAG, "WebUpdateManager initialized");
    initGuard.SetReady();
}

void WebUpdateManager::Tick(TickContext &ctx) {
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if(lastCheck != 0){
        if (!ctx.ElapsedAndReset(lastCheck, CHECK_INTERVAL))
            return;
    }
    else
    {
        lastCheck = NowMs();
    }


    ESP_LOGI(TAG, "Checking for web bundle update...");

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

    if (HashChanged(localHash, remoteHash)) {
        ESP_LOGI(TAG, "New web bundle detected, updating...");
        if (DownloadAndExtractBundle()) {
            WriteLocalManifest(remoteJson);
            ESP_LOGI(TAG, "Web bundle updated successfully");
        } else {
            ESP_LOGW(TAG, "Failed to update web bundle");
        }
    } else {
        ESP_LOGI(TAG, "Web bundle up to date");
    }
}

bool WebUpdateManager::FetchManifest(std::string &jsonOut) {
    esp_http_client_config_t config = {
        .url = "https://github.com/vanBassum/Thermy-ui/releases/download/latest/manifest.json",
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    if (esp_http_client_open(client, 0) != ESP_OK) {
        esp_http_client_cleanup(client);
        return false;
    }

    char buf[512];
    int total = 0;
    while (true) {
        int len = esp_http_client_read(client, buf, sizeof(buf));
        if (len <= 0) break;
        jsonOut.append(buf, len);
        total += len;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return total > 0;
}

bool WebUpdateManager::ParseManifest(const std::string &json, std::string &remoteHash) {
    cJSON *root = cJSON_Parse(json.c_str());
    if (!root) return false;
    cJSON *sha = cJSON_GetObjectItem(root, "sha256");
    if (sha && cJSON_IsString(sha))
        remoteHash = sha->valuestring;
    cJSON_Delete(root);
    return !remoteHash.empty();
}

bool WebUpdateManager::ReadLocalHash(std::string &hashOut) {
    FILE *f = fopen("/fat/manifest.json", "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    std::string content(size, 0);
    fread(content.data(), 1, size, f);
    fclose(f);
    return ParseManifest(content, hashOut);
}

bool WebUpdateManager::WriteLocalManifest(const std::string &json) {
    FILE *f = fopen("/fat/manifest.json", "w");
    if (!f) return false;
    fwrite(json.data(), 1, json.size(), f);
    fclose(f);
    return true;
}

bool WebUpdateManager::HashChanged(const std::string &local, const std::string &remote) {
    return local != remote;
}

bool WebUpdateManager::DownloadAndExtractBundle()
{
    esp_http_client_config_t cfg = {
        .url = "https://github.com/vanBassum/Thermy-ui/releases/download/latest/webbundle.tar",
        .timeout_ms = 60000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;

    if (esp_http_client_open(client, 0) != ESP_OK) {
        esp_http_client_cleanup(client);
        return false;
    }

    uint8_t buf[1024];
    TarExtractor tar("/fat");

    bool ok = true;
    while (true) {
        int n = esp_http_client_read(client, (char*)buf, sizeof(buf));
        if (n <= 0) break;
        tar.feed(buf, n);
    }

    tar.finish();
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return ok;
}
