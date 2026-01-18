/**
 *
 *  @file Result.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#ifndef VIX_UTILS_RESULT_HPP
#define VIX_UTILS_RESULT_HPP

#include <string>
#include <utility>
#include <type_traits>
#include <new>
#include <cassert>

/**
 * @brief Generic `Result<T, E>` type for error handling without exceptions.
 *
 * This header defines a lightweight `Result<T, E>` class template, inspired by Rust’s
 * `Result` and similar functional-style constructs. It represents either a success (`Ok`)
 * containing a value of type `T`, or a failure (`Err`) containing an error of type `E`.
 *
 * This allows developers to propagate and handle errors safely without relying
 * on exceptions, enabling deterministic, low-overhead control flow.
 *
 * ---
 * ### Example
 * @code
 * using namespace Vix::utils;
 *
 * Result<int> divide(int a, int b) {
 *     if (b == 0)
 *         return Result<int>::Err("division by zero");
 *     return Result<int>::Ok(a / b);
 * }
 *
 * int main() {
 *     auto res = divide(10, 2);
 *     if (res.is_ok()) {
 *         std::cout << "Result: " << res.value() << std::endl;
 *     } else {
 *         std::cerr << "Error: " << res.error() << std::endl;
 *     }
 * }
 * @endcode
 *
 * ---
 * @tparam T The success type.
 * @tparam E The error type (defaults to std::string).
 */

namespace vix::utils
{
  /** @brief Tag type used to construct a successful result. */
  struct OkTag
  {
    explicit OkTag() = default;
  };

  /** @brief Tag type used to construct an error result. */
  struct ErrTag
  {
    explicit ErrTag() = default;
  };

  /** @brief Constant tag for Ok construction. */
  inline constexpr OkTag OkTag_v{};

  /** @brief Constant tag for Err construction. */
  inline constexpr ErrTag ErrTag_v{};

  // Result<T, E> — Generic variant
  /**
   * @brief A type representing either a success (`Ok`) or an error (`Err`).
   *
   * This version stores a value (`T`) or an error (`E`) in a union,
   * ensuring compact storage and no dynamic allocations by default.
   *
   * Example:
   * @code
   * Result<std::string> read_config() {
   *     if (std::filesystem::exists("config.json"))
   *         return Result<std::string>::Ok("config.json");
   *     return Result<std::string>::Err("file not found");
   * }
   * @endcode
   */
  template <typename T, typename E = std::string>
  class Result
  {
    bool ok_;
    union
    {
      T val_;
      E err_;
    };

    explicit Result(OkTag, const T &v) : ok_(true), val_(v) {}
    explicit Result(OkTag, T &&v) : ok_(true), val_(std::move(v)) {}
    explicit Result(ErrTag, const E &e) : ok_(false), err_(e) {}
    explicit Result(ErrTag, E &&e) : ok_(false), err_(std::move(e)) {}

  public:
    static Result Ok(T v) { return Result(OkTag_v, std::move(v)); }
    static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

    Result(const Result &o) : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(val_)) T(o.val_);
      else
        ::new (std::addressof(err_)) E(o.err_);
    }

    Result(Result &&o) noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_move_constructible_v<E>)
        : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(val_)) T(std::move(o.val_));
      else
        ::new (std::addressof(err_)) E(std::move(o.err_));
    }

    ~Result()
    {
      if (ok_)
        val_.~T();
      else
        err_.~E();
    }

    Result &operator=(const Result &o)
    {
      if (this == &o)
        return *this;

      if (ok_ && o.ok_)
      {
        val_ = o.val_;
      }
      else if (!ok_ && !o.ok_)
      {
        err_ = o.err_;
      }
      else if (ok_ && !o.ok_)
      {
        val_.~T();
        ::new (std::addressof(err_)) E(o.err_);
        ok_ = false;
      }
      else
      {
        err_.~E();
        ::new (std::addressof(val_)) T(o.val_);
        ok_ = true;
      }
      return *this;
    }

    Result &operator=(Result &&o) noexcept(
        std::is_nothrow_move_assignable_v<T> &&
        std::is_nothrow_move_assignable_v<E> &&
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_move_constructible_v<E>)
    {
      if (this == &o)
        return *this;

      if (ok_ && o.ok_)
      {
        val_ = std::move(o.val_);
      }
      else if (!ok_ && !o.ok_)
      {
        err_ = std::move(o.err_);
      }
      else if (ok_ && !o.ok_)
      {
        val_.~T();
        ::new (std::addressof(err_)) E(std::move(o.err_));
        ok_ = false;
      }
      else
      {
        err_.~E();
        ::new (std::addressof(val_)) T(std::move(o.val_));
        ok_ = true;
      }
      return *this;
    }

    bool is_ok() const noexcept { return ok_; }
    bool is_err() const noexcept { return !ok_; }
    const T &value() const
    {
      assert(ok_);
      return val_;
    }

    T &value()
    {
      assert(ok_);
      return val_;
    }

    const E &error() const
    {
      assert(!ok_);
      return err_;
    }

    E &error()
    {
      assert(!ok_);
      return err_;
    }

    static Result FromOk(const T &v) { return Result(OkTag_v, v); }
    static Result FromOk(T &&v) { return Result(OkTag_v, std::move(v)); }
    static Result FromErr(const E &e) { return Result(ErrTag_v, e); }
    static Result FromErr(E &&e) { return Result(ErrTag_v, std::move(e)); }
    Result() = delete;
  };

  /**
   * @brief Specialization of Result for functions that return `void` on success.
   *
   * @code
   * Result<void> write_file(const std::string& path) {
   *     if (!has_permission(path))
   *         return Result<void>::Err("Permission denied");
   *     return Result<void>::Ok();
   * }
   * @endcode
   */
  template <typename E>
  class Result<void, E>
  {
    bool ok_;
    union
    {
      char dummy_; // Active when ok_ == true
      E err_;
    };

    explicit Result(OkTag) : ok_(true), dummy_(0) {}
    explicit Result(ErrTag, const E &e) : ok_(false), err_(e) {}
    explicit Result(ErrTag, E &&e) : ok_(false), err_(std::move(e)) {}

  public:
    static Result Ok() { return Result(OkTag_v); }
    static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

    Result(const Result &o) : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(dummy_)) char(0);
      else
        ::new (std::addressof(err_)) E(o.err_);
    }

    Result(Result &&o) noexcept(std::is_nothrow_move_constructible_v<E>)
        : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(dummy_)) char(0);
      else
        ::new (std::addressof(err_)) E(std::move(o.err_));
    }

    ~Result()
    {
      if (!ok_)
        err_.~E();
    }

    Result &operator=(const Result &o)
    {
      if (this == &o)
        return *this;

      if (ok_ && o.ok_)
      {
      }
      else if (!ok_ && !o.ok_)
      {
        err_ = o.err_;
      }
      else if (ok_ && !o.ok_)
      {
        ::new (std::addressof(err_)) E(o.err_);
        ok_ = false;
      }
      else
      {
        err_.~E();
        ::new (std::addressof(dummy_)) char(0);
        ok_ = true;
      }
      return *this;
    }

    Result &operator=(Result &&o) noexcept(
        std::is_nothrow_move_assignable_v<E> &&
        std::is_nothrow_move_constructible_v<E>)
    {
      if (this == &o)
        return *this;

      if (ok_ && o.ok_)
      {
      }
      else if (!ok_ && !o.ok_)
      {
        err_ = std::move(o.err_);
      }
      else if (ok_ && !o.ok_)
      {
        ::new (std::addressof(err_)) E(std::move(o.err_));
        ok_ = false;
      }
      else
      {
        err_.~E();
        ::new (std::addressof(dummy_)) char(0);
        ok_ = true;
      }
      return *this;
    }

    bool is_ok() const noexcept { return ok_; }
    bool is_err() const noexcept { return !ok_; }

    const E &error() const
    {
      assert(!ok_);
      return err_;
    }

    E &error()
    {
      assert(!ok_);
      return err_;
    }

    Result() = delete;
  };

} // namespace vix::utils

#endif // VIX_RESULT_HPP
