#include "Device.hpp"
#include <ostream>

namespace mg {

std::string Device::Str() const {
    std::string s = DeviceTypeName(Type());
    if (HasIdx()) {
        s.push_back(':');
        s.append(std::to_string(Idx()));
    }
    return s;
}

std::ostream& operator<<(std::ostream& stream, const Device& device) {
    stream << device.Str();
    return stream;
}

}