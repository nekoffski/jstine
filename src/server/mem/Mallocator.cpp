#include "Mallocator.hh"

#include "core/Log.hh"

namespace jstine {

[[nodiscard]] void* Mallocator::allocate(u64 size) {
    auto ptr = std::malloc(size);
    log::debug("mallocator: allocated {}b at {}", size, ptr);
    return ptr;
}

void Mallocator::free(void* ptr) {
    log::debug("mallocator: freeing {}", ptr);
    std::free(ptr);
}

}  // namespace jstine
