#include "SettingsManager.h"
#include <cassert>
#include "NvsSettingsLoader.h"
#include "NvsSettingsStorer.h"
#include "SettingsDefaultApplier.h"
#include "SettingsPrinter.h"
#include "SettingsPathResolver.h"
#include "SettingsStringSetter.h"
#include "SettingsStringGetter.h"


SettingsManager::SettingsManager(ServiceProvider& services)
{
    // Currently unused but allows future dependency use
}

void SettingsManager::Init()
{
    if(initGuard.IsReady())
        return;
    LOCK(mutex);

    // Load all settings
    NvsSettingsLoader loader(NVS_PARTITION);
    loader.Load(rootSettings);

    initGuard.SetReady();
}

// --------------------------------------------------
// Load and Save All
// --------------------------------------------------

void SettingsManager::LoadAll()
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    NvsSettingsLoader loader(NVS_PARTITION);
    loader.Load(rootSettings);
}

void SettingsManager::SaveAll()
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    NvsSettingsStorer storer(NVS_PARTITION);
    storer.Store(rootSettings);
}

// --------------------------------------------------
// Load and Save Individual Groups
// --------------------------------------------------

void SettingsManager::LoadGroup(ISettingsGroup &group)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    const char* name = FindGroupName(&group);
    assert(name && "Unknown settings group passed to LoadGroup");

    NvsSettingsLoader loader(NVS_PARTITION);
    loader.LoadGroup(group, name);
}

void SettingsManager::SaveGroup(const ISettingsGroup &group)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    const char* name = FindGroupName(&group);
    assert(name && "Unknown settings group passed to SaveGroup");

    NvsSettingsStorer storer(NVS_PARTITION);
    storer.StoreGroup(group, name);
}

// --------------------------------------------------
// Reset Handling
// --------------------------------------------------

void SettingsManager::ResetToFactoryDefaults()
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    rootSettings.IterateSettings([&](SettingHandle h)
    {
        if (h.Type() != SettingType::SettingGroup)
        {
            ESP_LOGE(TAG, "ResetToFactoryDefaults: Non-group setting '%s' at root level", h.Key());
            assert(false && "Non-group setting at root level");
            return;
        }

        ISettingsGroup& group = h.AsGroup();
        ResetGroupToFactoryDefaults(group);
    });
}

void SettingsManager::ResetGroupToFactoryDefaults(ISettingsGroup &group)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);
    SettingsDefaultApplier::ApplyGroup(group);
}

// --------------------------------------------------
// Utility
// --------------------------------------------------

void SettingsManager::Print(ISettingsGroup &group)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);
    SettingsPrinter printer;
    printer.PrintGroup(group);
}

bool SettingsManager::SetValueByString(const char* path, const char* value)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    SettingHandle handle;
    SettingsPathResolver resolver;
    if (!resolver.Resolve(rootSettings, path, handle))
        return false;

    StringConverter stringConverter;
    SettingsStringSetter setter(stringConverter);
    
    return setter.ApplyStringValue(handle, value);
}

bool SettingsManager::GetValueAsString(const char *path, char *outBuf, size_t outBufSize) const
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);
    
    SettingHandle handle;
    SettingsPathResolver resolver;
    if (!resolver.Resolve(rootSettings, path, handle))
        return false;
    StringConverter stringConverter;
    SettingsStringGetter getter(stringConverter);
    return getter.RetrieveStringValue(handle, outBuf, outBufSize);
}



// --------------------------------------------------
// Helper to resolve group name from RootSettings
// --------------------------------------------------

const char* SettingsManager::FindGroupName(const ISettingsGroup* groupPtr) const
{
    const SettingsDescriptor* schema = rootSettings.GetSettingsSchema();
    int count = rootSettings.GetSettingsCount();

    for (int i = 0; i < count; ++i)
    {
        const SettingsDescriptor& desc = schema[i];
        if (desc.type == SettingType::SettingGroup)
        {
            const uint8_t* base = reinterpret_cast<const uint8_t*>(&rootSettings);
            const ISettingsGroup* candidate = reinterpret_cast<const ISettingsGroup*>(base + desc.offset);
            if (candidate == groupPtr)
                return desc.key;
        }
    }

    return nullptr; // Not found
}
