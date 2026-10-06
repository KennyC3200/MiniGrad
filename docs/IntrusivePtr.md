# Intrusive Pointer
An intrusive pointer is a reference-counting smart pointer that stores the metadata (reference count) with the actual data itself rather than in an external control block. This is contrary to `std::shared_ptr`, which stores the reference count in an external control block.

Benefits:
- One heap allocation instead of two
- A smaller memory footprint, since an intrusive pointer only contains a raw pointer whereas a `std::shared_ptr` requires two pointers (one to the raw data, one to the control block)
- Safer creation from raw pointers
    - A common problem with `std::shared_ptr` is that there could be double deletes
- Better cache locality

Drawbacks:
- Classes must know they will be managed by an intrusive pointer
- No standard library support