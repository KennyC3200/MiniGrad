#pragma once

#include "DeviceType.hpp"
#include "Exception.hpp"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <functional>

namespace mg {

using DeviceIdx = int8_t;

struct Device final {
public:
    using Type = DeviceType;

    Device(DeviceType type, DeviceIdx idx = -1)
        : m_type(type), m_idx(idx)
    { 
        Validate(); 
    }

    bool operator==(const Device& rhs) const noexcept {
        return m_type == rhs.m_type && m_idx == rhs.m_idx;
    }

    bool operator!=(const Device& rhs) const noexcept { return !(*this == rhs); }

    void SetIdx(DeviceIdx idx) { 
        m_idx = idx; 
        Validate();
    }

    mg::DeviceType GetType() const noexcept { return m_type; }
    DeviceIdx Idx() const noexcept { return m_idx; }
    bool HasIdx() const noexcept { return m_idx != -1; }

    bool IsCPU() const noexcept { return m_type == DeviceType::CPU; }
    bool IsCUDA() const noexcept { return m_type == DeviceType::CUDA; }

    std::string Str() const;

private:
    mg::DeviceType m_type;
    mg::DeviceIdx m_idx = -1;

    void Validate() const {
        // Asserted only if MINIGRAD_DEBUG is defined
        INTERNAL_ASSERT_DEBUG_ONLY(
            m_idx >= -1,
            "Device idx must be -1 or non-negative, received {}",
            static_cast<int>(m_idx));

        // Enforce that CPU device index can only be -1 or 0
        INTERNAL_ASSERT_DEBUG_ONLY(
            !IsCPU() || m_idx <= 0,
            "CPU index must be -1 or zero, received {}",
            static_cast<int>(m_idx));
    }
};

std::ostream& operator<<(std::ostream& stream, const Device& device);

}

namespace std {

template<>
struct hash<mg::Device> {
    std::size_t operator()(const mg::Device& device) const noexcept {
        uint32_t bits = static_cast<uint32_t>(static_cast<uint8_t>(device.GetType())) << 16
            | static_cast<uint32_t>(static_cast<uint8_t>(device.Idx()));
        return std::hash<uint32_t>{}(bits);
    }
};

}