#pragma once

#include <memory>

#include "Storage.hh"
#include "core/Concepts.hh"
#include "core/Config.hh"
#include "core/Core.hh"
#include "core/Error.hh"

namespace jstine {

class StorageManager : public NonCopyable, public NonMovable {
   public:
    virtual ~StorageManager() = default;

    virtual bool exists(const Key& key) const = 0;
    virtual void remove(const Key& key) = 0;
    virtual Opt<Error> set(const Key& key, const Value& value) = 0;
    virtual Result<Value> get(const Key& key) const = 0;

    static std::unique_ptr<StorageManager> create(const Config& config);
};

}  // namespace jstine
