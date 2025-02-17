// Copyright (c) 2022, Tyler Weaver
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cxxabi.h>
#include <fmt/format.h>

#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "fp/_external/expected.hpp"
#include "fp/no_discard.hpp"

namespace fp {

/**
 * @brief      Enum for ErrorCodes inspired by absl::StatusCode
 */
enum class ErrorCode : int {
  UNKNOWN,
  CANCELLED,
  INVALID_ARGUMENT,
  TIMEOUT,
  NOT_FOUND,
  ALREADY_EXISTS,
  PERMISSION_DENIED,
  RESOURCE_EXHAUSTED,
  FAILED_PRECONDITION,
  ABORTED,
  OUT_OF_RANGE,
  UNIMPLEMENTED,
  INTERNAL,
  UNAVAILABLE,
  DATA_LOSS,
  UNAUTHENTICATED,
  EXCEPTION,
};

/**
 * @brief      Error type used by Result<T>
 */
struct [[nodiscard]] Error {
  ErrorCode code = ErrorCode::UNKNOWN;
  std::string what = "";

  inline bool operator==(const Error& other) const noexcept {
    return code == other.code && what == other.what;
  }
  inline bool operator!=(const Error& other) const noexcept {
    return code != other.code || what != other.what;
  }
};

Error Unknown(const std::string& what = "");
Error Cancelled(const std::string& what = "");
Error InvalidArgument(const std::string& what = "");
Error Timeout(const std::string& what = "");
Error NotFound(const std::string& what = "");
Error AlreadyExists(const std::string& what = "");
Error PermissionDenied(const std::string& what = "");
Error ResourceExhausted(const std::string& what = "");
Error FailedPrecondition(const std::string& what = "");
Error Aborted(const std::string& what = "");
Error OutOfRange(const std::string& what = "");
Error Unimplemented(const std::string& what = "");
Error Internal(const std::string& what = "");
Error Unavailable(const std::string& what = "");
Error DataLoss(const std::string& what = "");
Error Unauthenticated(const std::string& what = "");
Error Exception(const std::string& what = "");

/**
 * @brief      convert ErrorCode to string_view for easy formatting
 *
 * @param[in]  code  The error code
 */
[[nodiscard]] constexpr std::string_view toStringView(const ErrorCode& code) {
  switch (code) {
    case ErrorCode::CANCELLED:
      return "Cancelled";
    case ErrorCode::UNKNOWN:
      return "Unknown";
    case ErrorCode::INVALID_ARGUMENT:
      return "InvalidArgument";
    case ErrorCode::TIMEOUT:
      return "Timeout";
    case ErrorCode::NOT_FOUND:
      return "NotFound";
    case ErrorCode::ALREADY_EXISTS:
      return "AlreadyExists";
    case ErrorCode::PERMISSION_DENIED:
      return "PermissionDenied";
    case ErrorCode::RESOURCE_EXHAUSTED:
      return "ResourceExhausted";
    case ErrorCode::FAILED_PRECONDITION:
      return "FailedPrecondition";
    case ErrorCode::ABORTED:
      return "Aborted";
    case ErrorCode::OUT_OF_RANGE:
      return "OutOfRange";
    case ErrorCode::UNIMPLEMENTED:
      return "Unimplemented";
    case ErrorCode::INTERNAL:
      return "Internal";
    case ErrorCode::UNAVAILABLE:
      return "Unavailable";
    case ErrorCode::DATA_LOSS:
      return "DataLoss";
    case ErrorCode::UNAUTHENTICATED:
      return "Unauthenticated";
    case ErrorCode::EXCEPTION:
      return "Exception";
    default:
      __builtin_unreachable();
  }
}

/**
 * Result<T> type
 *
 * @example    result.cpp
 */
template <typename T, typename E = Error>
using Result = tl::expected<T, E>;

/**
 * @brief      Makes a Result<T> from a T value
 *
 * @param[in]  value  The value
 *
 * @tparam     T      The type of value
 *
 * @return     A Result<T> containing value
 */
template <typename T, typename E = Error>
constexpr Result<T, E> make_result(T value) {
  return Result<T, E>{value};
}

/**
 * @brief Filter function for testing if a result has an error
 *
 * @param exp The expected type to test
 * @tparam T The value type
 * @tparam E The error type
 * @return if the expected has an error
 */
template <typename T, typename E>
constexpr bool has_error(const tl::expected<T, E>& exp) {
  return !exp;
}

/**
 * @brief       Tests if any of the expected args passed in has an error.
 *
 * @param[in]   The tl::expected<T, E> variables.  All have to use the same
 * error type.
 * @tparam      E The error type
 * @tparam      Args The value types for the tl::expected<T, E> args
 * @return      The first error found or nothing
 * @example     maybe_error.cpp
 */
template <typename E, typename... Args>
constexpr std::optional<E> maybe_error(tl::expected<Args, E>... args) {
  auto maybe = std::optional<E>{std::nullopt};
  (
      [&](auto& exp) {
        if (maybe.has_value()) return;
        if (has_error(exp)) maybe = exp.error();
      }(args),
      ...);
  return maybe;
}

/**
 * @brief      Try to Result<T>.  Lifts a function that throws an excpetpion to
 * one that returns a Result<T>
 *
 * @param[in]  f     The function to call
 *
 * @tparam     F     The function type
 * @tparam     Ret   The return value of the function
 * @tparam     Exp   The expected type
 *
 * @return     The return value of the function
 */
template <typename F, typename Ret = typename std::result_of<F()>::type,
          typename Exp = Result<Ret>>
Exp try_to_result(F f) {
  try {
    return make_result(f());
  } catch (const std::exception& ex) {
    return tl::make_unexpected(Exception(fmt::format(
        "[{}: {}]", abi::__cxa_current_exception_type()->name(), ex.what())));
  }
}

}  // namespace fp

/**
 * @brief      fmt format implementation for Error type
 */
template <>
struct fmt::formatter<fp::Error> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const fp::Error& error, FormatContext& ctx) {
    return format_to(ctx.out(), "[Error: [{}] {}]", toStringView(error.code),
                     error.what);
  }
};

/**
 * @brief      fmt format implementation for Result<T> type
 */
template <typename T>
struct fmt::formatter<fp::Result<T>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const fp::Result<T>& result, FormatContext& ctx) {
    if (result.has_value()) {
      return format_to(ctx.out(), "[Result<T>: value={}]", result.value());
    } else {
      return format_to(ctx.out(), "[Result<T>: {}]", result.error());
    }
  }
};
