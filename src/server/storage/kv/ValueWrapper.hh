#pragma once

#include "Value.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class ValueWrapper : public NonCopyable {
   public:
    explicit ValueWrapper();

    Opt<Error> set(CBytesView bytes, Allocator& allocator);

    CBytesView bytes() const;
    const Metadata& metadata() const;

   private:
    Metadata m_metadata;
    Value m_value;
};

}  // namespace jstine
