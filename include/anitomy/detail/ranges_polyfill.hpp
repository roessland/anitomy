#pragma once

// local-patch: libc++ (used by Emscripten / AppleClang) lacks
// std::views::enumerate (C++23, P2164) and std::views::adjacent
// (C++23, P2374). Both can be expressed in terms of std::views::zip
// (libc++ 15+) plus iota / drop. We expose them as plain function
// calls — `polyfill::enumerate(r)` and `polyfill::adjacent<N>(r)` —
// so call sites don't need pipe-closure machinery. Drop this header
// once libc++ ships these views (track __cpp_lib_ranges_enumerate
// and __cpp_lib_ranges_adjacent).

#include <cstddef>
#include <ranges>
#include <utility>

namespace anitomy::detail::polyfill {

inline constexpr auto enumerate = []<std::ranges::viewable_range R>(R&& r) {
  return std::views::zip(
      std::views::iota(std::ranges::range_difference_t<R>{0}),
      std::forward<R>(r));
};

namespace impl {

template <std::size_t N, typename View, std::size_t... I>
constexpr auto adjacent_zip(View view, std::index_sequence<I...>) {
  return std::views::zip((view | std::views::drop(I))...);
}

}  // namespace impl

template <std::size_t N>
inline constexpr auto adjacent = []<std::ranges::viewable_range R>(R&& r) {
  auto view = std::views::all(std::forward<R>(r));
  return impl::adjacent_zip<N>(view, std::make_index_sequence<N>{});
};

}  // namespace anitomy::detail::polyfill
