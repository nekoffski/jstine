#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"

namespace jstine {

class Allocator : public NonCopyable, public NonMovable {
   public:
    virtual ~Allocator() = default;

    [[nodiscard]] virtual void* allocate(u64 size) = 0;
    virtual void free(void* ptr) = 0;
};

}  // namespace jstine
