#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "SettingsDescriptor.h"
#include "ISettingsGroup.h"

// ---------------------------------------------------------------------
// Helpers for better static_assert messages (show deduced type names)
// ---------------------------------------------------------------------
template<typename T>
struct always_false : std::false_type {};

// ---------------------------------------------------------------------
// Safe OFFSET_OF replacement (no GCC -Winvalid-offsetof warnings)
// ---------------------------------------------------------------------
#define OFFSET_OF(type, member) \
    (static_cast<size_t>(reinterpret_cast<const uint8_t*>(&(reinterpret_cast<const type*>(0)->member)) - \
                         reinterpret_cast<const uint8_t*>(0)))


// ---------------------------------------------------------------------
// Descriptor expansion macros
// ---------------------------------------------------------------------
#define DESCRIPTOR_GROUP(key, field) \
    { key, SettingType::SettingGroup, OFFSET_OF(ThisType, field), sizeof(field), {} }

#define DESCRIPTOR_FIELD(key, field, def) \
{ \
    key, \
    DeduceSettingType<decltype(((ThisType*)nullptr)->field)>(), \
    OFFSET_OF(ThisType, field), \
    sizeof(((ThisType*)nullptr)->field), \
    DeduceDefaultValue<decltype(((ThisType*)nullptr)->field)>(def) \
}






// ---------------------------------------------------------------------
// constexpr SettingType deduction
// ---------------------------------------------------------------------
template<typename FieldT>
constexpr SettingType DeduceSettingType()
{
    using T = std::remove_cv_t<std::remove_reference_t<FieldT>>;

    if constexpr (std::is_same_v<T, bool>)
        return SettingType::Boolean;
    else if constexpr (std::is_floating_point_v<T>)
        return SettingType::Float;
    else if constexpr (std::is_enum_v<T>)
        return SettingType::Enum;
    else if constexpr (std::is_same_v<T, int8_t>)
        return SettingType::Integer8;
    else if constexpr (std::is_same_v<T, uint8_t>)
        return SettingType::Unsigned8;
    else if constexpr (std::is_same_v<T, int16_t>)
        return SettingType::Integer16;
    else if constexpr (std::is_same_v<T, uint16_t>)
        return SettingType::Unsigned16;
    else if constexpr (std::is_same_v<T, int32_t>)
        return SettingType::Integer32;
    else if constexpr (std::is_same_v<T, uint32_t>)
        return SettingType::Unsigned32;
    else if constexpr (std::is_array_v<T>) {
        using E = std::remove_extent_t<T>;
        if constexpr (std::is_same_v<E, char>)
            return SettingType::String;
        else if constexpr (std::is_same_v<E, uint8_t>)
            return SettingType::Blob;
        else
            static_assert(always_false<E>::value,
                          "Invalid array element type: only char[] or uint8_t[] allowed");
    }
    else
        static_assert(always_false<T>::value, "Unsupported field type in DESCRIPTOR_FIELD");

    // Prevent compiler warning
    return SettingType::Blob;
}

// ---------------------------------------------------------------------
// constexpr DefaultValue deduction
// ---------------------------------------------------------------------
template<typename FieldT, typename DefaultT>
constexpr DefaultValue DeduceDefaultValue(const DefaultT& def)
{
    using T = std::remove_cv_t<std::remove_reference_t<FieldT>>;

    if constexpr (std::is_array_v<T>) {
        using E = std::remove_extent_t<T>;
        if constexpr (std::is_same_v<E, char>)
            return DefaultValue{ .str = def };
        else if constexpr (std::is_same_v<E, uint8_t>)
            return DefaultValue{ .blob = def };
        else
            static_assert(always_false<E>::value,
                          "Only char[] or uint8_t[] arrays are allowed for defaults");
    }
    else if constexpr (std::is_same_v<T, bool>)
        return DefaultValue{ .b = static_cast<bool>(def) };
    else if constexpr (std::is_floating_point_v<T>)
        return DefaultValue{ .f = static_cast<float>(def) };
    else if constexpr (std::is_enum_v<T> || std::is_integral_v<T>)
        return DefaultValue{ .i = static_cast<int32_t>(def) };
    else
        static_assert(always_false<T>::value, "Unsupported field type in DEDUCE_DEFAULT_VALUE");

    return {};
}
