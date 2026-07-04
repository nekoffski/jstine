#pragma once

#include <concepts>
#include <utility>

namespace jstine {

struct StaticClass {
    StaticClass() = delete;
};

struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(NonCopyable const&) = delete;
    NonCopyable& operator=(NonCopyable const&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};

struct NonMovable {
    NonMovable() = default;
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

namespace detail {
template <typename C, typename Signature>
struct CallableHelper;

template <typename C, typename R, typename... Args>
struct CallableHelper<C, R(Args...)> {
    static constexpr bool value = requires(C&& c, Args&&... args) {
        { c(std::forward<Args>(args)...) } -> std::same_as<R>;
    };
};
}  // namespace detail

template <typename C, typename Signature>
concept Callable = detail::CallableHelper<C, Signature>::value;

template <typename T, typename... Ts>
concept OneOf = requires() {
    { (std::is_same_v<T, Ts> || ...) };
};

template <typename T>
concept Iterable = requires(T& t) {
    { t.begin() };
    { t.end() };
    { t.cbegin() };
    { t.cend() };
};

template <typename T>
concept Countable = requires(T& t) {
    { t.size() };
};

}  // namespace jstine
