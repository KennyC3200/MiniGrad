#include "DeviceType.hpp"

namespace mg {

std::string DeviceTypeName(DeviceType type) {
    switch (type) {
        case DeviceType::CPU:
            return "CPU";
        case DeviceType::CUDA:
            return "CUDA";
        case DeviceType::NUM_DEVICE_TYPES:
            return "NUM_DEVICE_TYPES";
        default:
            return "";
    }
}

}