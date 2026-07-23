#pragma once

#include <variant>

#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"
#include "core/Time.hh"
#include "mem/Allocator.hh"

namespace jstine {

enum class ValueKind { str = 0, list = 1 };

struct Metadata {
    ValueKind kind;
    Clock::time_point accessedAt;
    u64 size;
};

class StrValueBody {
   public:
    explicit StrValueBody(std::span<const Byte> bytes, Allocator* allocator);
    ~StrValueBody();

    std::span<Byte> bytes();
    std::span<const Byte> bytes() const;

    StrValueBody(StrValueBody&&) noexcept;
    StrValueBody& operator=(StrValueBody&&) noexcept;

    StrValueBody& operator=(const StrValueBody&);

   private:
    Byte* m_ptr;
    u64 m_size;
    Allocator* m_allocator;
};

using ValueBody = std::variant<StrValueBody>;

class Value {
   public:
    static Result<Value> fromBytes(
        std::span<const Byte> bytes, Allocator& allocator
    );

    std::span<const Byte> bytes() const;
    const Metadata& metadata() const;

   private:
    explicit Value(ValueBody body);

    Metadata m_metadata;
    ValueBody m_body;
};

}  // namespace jstine
