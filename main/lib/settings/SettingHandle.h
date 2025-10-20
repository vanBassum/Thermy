#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include "SettingsDescriptor.h"
#include "ISettingsGroup.h"

class SettingHandle
{
    constexpr static const char* TAG = "SettingHandle";

private:
    const SettingsDescriptor* _desc = nullptr;
    ISettingsGroup* _owner = nullptr;

public:
    SettingHandle() = default;

    SettingHandle(const SettingsDescriptor& desc, ISettingsGroup& owner)
        : _desc(&desc), _owner(&owner)
    {}

    bool IsValid() const { return _desc && _owner; }

    const char* Key() const { assert(IsValid()); return _desc->key; }
    SettingType Type() const { assert(IsValid()); return _desc->type; }
    size_t Size() const { assert(IsValid()); return _desc->size; }
    const DefaultValue& Default() const { assert(IsValid()); return _desc->defaultVal; }

    void* Ptr()
    {
        assert(IsValid());
        return _desc->GetPtr(*_owner);
    }

    const void* Ptr() const
    {
        assert(IsValid());
        return _desc->GetPtr(*_owner);
    }

    // ----------------------------
    // Type-safe accessors
    // ----------------------------
    int8_t& AsInt8()                        { assert(Type() == SettingType::Integer8);  return ref_as<int8_t>(); }
    int16_t& AsInt16()                      { assert(Type() == SettingType::Integer16); return ref_as<int16_t>(); }
    int32_t& AsInt32()                      { assert(Type() == SettingType::Integer32 || Type() == SettingType::Enum); return ref_as<int32_t>(); }
    uint8_t& AsUInt8()                      { assert(Type() == SettingType::Unsigned8);  return ref_as<uint8_t>(); }
    uint16_t& AsUInt16()                    { assert(Type() == SettingType::Unsigned16); return ref_as<uint16_t>(); }
    uint32_t& AsUInt32()                    { assert(Type() == SettingType::Unsigned32); return ref_as<uint32_t>(); }

    const int8_t& AsInt8() const            { assert(Type() == SettingType::Integer8);  return ref_as<int8_t>(); }
    const int16_t& AsInt16() const          { assert(Type() == SettingType::Integer16); return ref_as<int16_t>(); }
    const int32_t& AsInt32() const          { assert(Type() == SettingType::Integer32 || Type() == SettingType::Enum); return ref_as<int32_t>(); }
    const uint8_t& AsUInt8() const          { assert(Type() == SettingType::Unsigned8);  return ref_as<uint8_t>(); }
    const uint16_t& AsUInt16() const        { assert(Type() == SettingType::Unsigned16); return ref_as<uint16_t>(); }
    const uint32_t& AsUInt32() const        { assert(Type() == SettingType::Unsigned32); return ref_as<uint32_t>(); }

    bool& AsBool()                          { assert(Type() == SettingType::Boolean); return ref_as<bool>(); }
    const bool& AsBool() const              { assert(Type() == SettingType::Boolean); return ref_as<bool>(); }

    float& AsFloat()                        { assert(Type() == SettingType::Float); return ref_as<float>(); }
    const float& AsFloat() const            { assert(Type() == SettingType::Float); return ref_as<float>(); }

    char* AsString()                        { assert(Type() == SettingType::String); return reinterpret_cast<char*>(Ptr()); }
    const char* AsString() const            { assert(Type() == SettingType::String); return reinterpret_cast<const char*>(Ptr()); }

    void* AsBlob()                          { assert(Type() == SettingType::Blob); return Ptr(); }
    const void* AsBlob() const              { assert(Type() == SettingType::Blob); return Ptr(); }

    ISettingsGroup& AsGroup()               { assert(Type() == SettingType::SettingGroup); return ref_as<ISettingsGroup>(); }
    const ISettingsGroup& AsGroup() const   { assert(Type() == SettingType::SettingGroup); return ref_as<ISettingsGroup>(); }

private:
    template<typename T>
    T& ref_as() { return *reinterpret_cast<T*>(Ptr()); }

    template<typename T>
    const T& ref_as() const { return *reinterpret_cast<const T*>(Ptr()); }
};
