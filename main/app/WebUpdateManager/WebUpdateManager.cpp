#include "WebUpdateManager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include <vector>
#include <sys/stat.h>

// ------------------ Manager Implementation ------------------

WebUpdateManager::WebUpdateManager(ServiceProvider &ctx)
    : settingsManager(ctx.GetSettingsManager()) {}

void WebUpdateManager::Init()
{
    if (initGuard.IsReady())
        return;

    // Initialize HTTP client with no fixed base URL
    client.Init("https://github.com");

    // Load stored SHA from settings
    settingsManager.Access([this](RootSettings &settings)
    {
        strncpy(currentSha, settings.runtime.uiSha256, sizeof(currentSha) - 1);
        currentSha[sizeof(currentSha) - 1] = '\0';
    });

    ESP_LOGI(TAG, "WebUpdateManager initialized (current SHA: %s)", 
             currentSha[0] ? currentSha : "<none>");
    initGuard.SetReady();
}

void WebUpdateManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (!ctx.HasElapsed(lastCheck, CHECK_INTERVAL))
        return;

    ESP_LOGI(TAG, "Checking for web bundle update...");
    DownloadIfRequired();

    ctx.MarkExecuted(lastCheck, CHECK_INTERVAL);
}

void WebUpdateManager::DownloadIfRequired()
{
    static constexpr const char *MANIFEST_URL =
        "https://github.com/vanBassum/Thermy-ui/releases/download/latest/manifest.json";
    static constexpr const char *WEBBUNDLE_URL =
        "https://github.com/vanBassum/Thermy-ui/releases/download/latest/webbundle.tar";

    Manifest manifest;
    if (!GetManifest(MANIFEST_URL, manifest))
    {
        ESP_LOGW(TAG, "Failed to fetch manifest.");
        return;
    }

    if (strcmp(currentSha, manifest.sha256) == 0)
    {
        ESP_LOGI(TAG, "Web bundle is up to date (SHA: %s).", manifest.sha256);
        return;
    }

    ESP_LOGI(TAG, "New web bundle available!");
    ESP_LOGI(TAG, "Old SHA: %s", currentSha[0] ? currentSha : "<none>");
    ESP_LOGI(TAG, "New SHA: %s", manifest.sha256);

    if (!DownloadWebBundle(WEBBUNDLE_URL))
    {
        ESP_LOGW(TAG, "Failed to download web bundle.");
        return;
    }

    // Save new SHA
    settingsManager.Access([&](RootSettings &settings)
    {
        strncpy(settings.runtime.uiSha256, manifest.sha256, sizeof(settings.runtime.uiSha256) - 1);
        settings.runtime.uiSha256[sizeof(settings.runtime.uiSha256) - 1] = '\0';

        strncpy(currentSha, manifest.sha256, sizeof(currentSha) - 1);
        currentSha[sizeof(currentSha) - 1] = '\0';

        settingsManager.SaveAll();
    });

    ESP_LOGI(TAG, "Web bundle updated successfully. New SHA: %s", manifest.sha256);
}

bool WebUpdateManager::GetManifest(const char *manifestUrl, Manifest &out_manifest)
{
    HttpClientRequest request(client);
    request.SetPath(manifestUrl);
    request.SetMethod(HTTP_METHOD_GET);
    request.AddHeader("User-Agent", "Thermy-ESP32");
    request.Begin();
    request.Finalize();

    Stream &response = request.GetStream();
    char buffer[256];
    size_t len = response.read(buffer, sizeof(buffer) - 1);
    if (len == 0)
    {
        ESP_LOGW(TAG, "Failed to read manifest response");
        return false;
    }

    buffer[len] = '\0';

    cJSON *root = cJSON_Parse(buffer);
    if (!root)
    {
        ESP_LOGW(TAG, "Failed to parse manifest JSON");
        return false;
    }

    cJSON *sha256 = cJSON_GetObjectItem(root, "sha256");
    if (!cJSON_IsString(sha256))
    {
        ESP_LOGW(TAG, "Manifest missing 'sha256' field");
        cJSON_Delete(root);
        return false;
    }

    strncpy(out_manifest.sha256, sha256->valuestring, sizeof(out_manifest.sha256) - 1);
    out_manifest.sha256[sizeof(out_manifest.sha256) - 1] = '\0';

    ESP_LOGI(TAG, "Manifest SHA256: %s", out_manifest.sha256);
    cJSON_Delete(root);
    return true;
}

bool WebUpdateManager::DownloadWebBundle(const char *webbundleUrl)
{
    HttpClientRequest request(client);
    request.SetPath(webbundleUrl);
    request.SetMethod(HTTP_METHOD_GET);
    request.AddHeader("User-Agent", "Thermy-ESP32");
    request.Begin();
    request.Finalize();

    Stream &response = request.GetStream();

    // Stream to file (or SPIFFS/LittleFS) instead of RAM
    uint8_t buffer[1024];
    size_t total = 0;

    while (true)
    {
        size_t len = response.read(buffer, sizeof(buffer));
        if (len == 0)
            break;
        total += len;

        ESP_LOGI(TAG, "Downloaded %u bytes...", (unsigned)total);
        // TODO: write buffer to filesystem or flash partition
        // Example:
        // fs_write(file, buffer, len);
    }

    if (total == 0)
    {
        ESP_LOGW(TAG, "Downloaded zero bytes from %s", webbundleUrl);
        return false;
    }

    ESP_LOGI(TAG, "Downloaded %u bytes of webbundle.tar", (unsigned)total);
    return true;
}
