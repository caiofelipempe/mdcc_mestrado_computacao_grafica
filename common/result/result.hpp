// result.hpp
#pragma once

#include <expected>
#include <optional>
#include <type_traits>

template <typename T, typename E = void, bool Safe = true>
using Result =
    std::conditional_t<
        Safe,
        std::conditional_t<
            std::is_void_v<E>,
            std::optional<T>,
            std::expected<T, E>
        >,
        T
    >;

namespace result {

template <typename E>
constexpr std::unexpected<E> err(E error)
{
    return std::unexpected<E>{ std::move(error) };
}

constexpr std::nullopt_t none()
{
    return std::nullopt;
}

}