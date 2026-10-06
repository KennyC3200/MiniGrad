#include "CPUAllocator.hpp"
#include "Allocator.hpp"

namespace mg {

constexpr std::align_val_t CPU_ALLOC_ALIGN{64};

struct CPUAllocator final : Allocator {
    CPUAllocator() = default;

    virtual DataPtr Allocate(std::size_t n) override {
        if (n == 0) return DataPtr();

        // TODO: Maybe in the future if we want to throw a different exception, since `bad_alloc`
        // propagates out of `Allocate` identically whether the try/catch is present or not
        void* ptr = nullptr;
        try {
            // CPU tensor data is allocated aligned to a fixed boundary (historically 64 bytes)
            ptr = ::operator new(n, CPU_ALLOC_ALIGN);
        } catch (const std::bad_alloc&) {
            throw;
        }

        return {ptr, ptr, &Delete, Device(DeviceType::CPU)};
    }

    static void Delete(void* ptr) {
        if (!ptr) return;
        ::operator delete(ptr, CPU_ALLOC_ALIGN);
    }
    
    void CopyData(void* dest, const void* src, std::size_t cnt) const final {
        return DefaultCopyData(dest, src, cnt);
    }
};

// Global CPU allocator
static CPUAllocator g_cpu_alloc;

// Register the CPU allocator in the global array of allocators
REGISTER_ALLOCATOR(DeviceType::CPU, &g_cpu_alloc);

void SetCPUAllocator(Allocator* allocator) {
    SetAllocator(DeviceType::CPU, allocator);
}

Allocator* GetCPUAllocator() {
    return &g_cpu_alloc;
}

}