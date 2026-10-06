#pragma once

#include "Allocator.hpp"

namespace mg {

Allocator* GetCPUAllocator();
void SetCPUAllocator(Allocator* allocator);

}