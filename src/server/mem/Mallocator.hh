#pragma once

#include "Allocator.hh"

namespace jstine {

class Mallocator : public Allocator {
   public:
    [[nodiscard]] void* allocate(u64 size) override;
    void free(void* ptr) override;
};

}  // namespace jstine
