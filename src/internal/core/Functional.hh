#pragma once

#include <functional>
#include <ranges>

#include "Concepts.hh"

namespace jstine {

namespace details {

template <typename T>
struct ToImpl {};

template <typename T, std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, T>
std::vector<T> operator|(R&& r, ToImpl<T>) {
    std::vector<T> out;

    if constexpr (std::ranges::sized_range<decltype(r)>) {
        out.reserve(std::ranges::size(r));
    }

    std::ranges::copy(r, std::back_inserter(out));
    return out;
}

}  // namespace details

template <typename T>
auto toVector() {
    return details::ToImpl<T>{};
}

template <typename... Ts>
struct Overloader : Ts... {
    using Ts::operator()...;
};

class GuardCall : public NonCopyable, public NonMovable {
   public:
    GuardCall() : m_callback([]() {}) {}

    template <typename Callback>
    GuardCall(Callback&& callback)
        : m_callback(std::forward<Callback>(callback)) {}

    ~GuardCall() { m_callback(); }

   private:
    std::function<void()> m_callback;
};

template <typename F>
    requires std::is_invocable_v<F>
class LazyEvaluator {
    using Result = decltype(std::declval<const F>()());

   public:
    explicit LazyEvaluator(F&& evaluable) : m_evaluable(std::move(evaluable)) {}

    constexpr operator Result() const { return m_evaluable(); }

    template <typename T>
        requires(std::is_constructible_v<T, Result>)
    constexpr operator T() const {
        return T{m_evaluable()};
    }

   private:
    F m_evaluable;
};

template <typename F>
constexpr auto lazyEvaluate(F&& f) {
    return LazyEvaluator{std::forward<F>(f)};
}

#define LAZY_EVALUATE(expr) lazyEvaluate([&]() { return (expr); })

}  // namespace jstine