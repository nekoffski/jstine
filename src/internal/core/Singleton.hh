#pragma once

#include "Concepts.hh"
#include "Core.hh"
#include "Log.hh"

namespace jstine {

template <typename T>
class Singleton : public virtual NonMovable, public virtual NonCopyable {
   public:
    [[nodiscard]] static T& get() {
        static T* instance = new T{};  // let it leak
        return *instance;
    }

    static void noop() {}
};

template <typename T>
class UniqueInstance : public virtual NonMovable, public virtual NonCopyable {
   public:
    explicit UniqueInstance() {
        log::expect(not s_instanceExists, "Instance already exists");
        s_instanceExists = true;
    }

   private:
    inline static bool s_instanceExists = false;
};

}  // namespace jstine
