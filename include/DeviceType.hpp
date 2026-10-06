#pragma once

#include <string>

namespace mg {

enum class DeviceType : int8_t {
    CPU = 0,
    CUDA = 1,
    NUM_DEVICE_TYPES = 2
};

constexpr int NUM_DEVICE_TYPES = static_cast<int>(DeviceType::NUM_DEVICE_TYPES);

std::string DeviceTypeName(DeviceType type);

}