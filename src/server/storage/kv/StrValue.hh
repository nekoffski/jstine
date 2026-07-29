#pragma once

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "mem/Allocator.hh"

namespace jstine {

class StrValue : public NonCopyable {
   public:
    CBytesView bytes() const;

    static Result<StrValue> parse(Allocator* allocator, CBytesView input);

    StrValue(StrValue&& other) noexcept;
    StrValue& operator=(StrValue&& other) noexcept;

    ~StrValue();

   private:
    explicit StrValue(Allocator* allocator, u64 size, Byte* bytes);

    Allocator* m_allocator;
    u64 m_size;
    Byte* m_bytes;
};

}  // namespace jstine
