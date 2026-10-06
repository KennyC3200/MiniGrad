#include "UniqueVoidPtr.hpp"
#include <iostream>

void PtrCtxSameType() {
    void* raw = ::operator new(1024);
    mg::UniqueVoidPtr ptr(
        raw, raw, 
        [](void* p) { 
            ::operator delete(p); 
            std::cout << "PtrCtxSameType: delete successful" << std::endl;
        });

    void* usable = ptr.Get();
    void* ctx = ptr.GetContext();
}

constexpr std::size_t CUDA_OFFSET = 256;

void* cuda_malloc(std::size_t n) { 
    return ::operator new(n); 
}
void cuda_free(void* block) { 
    ::operator delete(block);
}

void PtrCtxDiffType() {
    char* block = static_cast<char*>(cuda_malloc(4096));
    void* offset = block + 256;

    mg::UniqueVoidPtr ptr(
        offset,
        block,
        [](void* p) { 
            cuda_free(p);
            std::cout << "PtrCtxDiffType: delete successful" << std::endl;
        }
    );
}

int main() {
    PtrCtxSameType();
    PtrCtxDiffType();
}