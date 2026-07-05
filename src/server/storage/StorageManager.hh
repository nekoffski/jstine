#pragma once

#include "Storage.hh"
#include "core/Concepts.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class StorageManager : public NonCopyable, public NonMovable {
   public:
    bool exists(const Key& key) const;
    void remove(const Key& key);
    Opt<Error> set(const Key& key, const Value& value);

   private:
};

}  // namespace jstine
