# Unique Void Pointer
This smart pointer exists to solve some issues that exist with `std::unique_ptr`. The main issue is that `std::unique_ptr<void, Deleter>` ties one pointer to one deleter. That is, whatever pointer you store is the pointer that gets passed to the deleter on the destruction. Allocators oftentimes needs to hand out a pointer that's different from the pointer that needs to be freed. For example:
- A CUDA caching allocator could sub-allocate. The allocator would return a pointer somewhere inside the larger cache block, but freeing requires a different pointer, namely the block's base pointer
- Shared memory cases hand back a pointer to mapped memory, however freeing requires a different pointer that points to the original mapping context
And `std::unique_ptr` can handle this, by storing extra content in the deleter itself so that you can delete the right thing. The issue with `std::unique_ptr` is that if the data pointer is `nullptr`, the deleter will not be called--leading to a memory leak if the context is non-null (and the deleter is responsible for freeing both the data and the context pointer).

To solve this, simply store the context with the pointer itself and attach the deleter to the context pointer. In most cases, the context points to the pointer.

## Under the Hood
The class has two members:
```cpp
void* m_data;
std::unique_ptr<void, DeleterFnPtr> m_ctx;
```
Where `m_data` is what is the data being dereferenced, and `m_ctx` is a unique pointer that often (but not always) points to the same `m_data` and has a custom deleter with type `using DeleterFnPtr = void (*)(void*)` which knows how to release it. Since `m_ctx` is a `std::unique_ptr`, move-only ownership and automatic destruction apply automatically; `m_ctx`'s move constructor and assignment default to moving `m_ctx` and copying `m_data`.

## Examples
See [here](../examples/ExampleUniqueVoidPtr.cpp).