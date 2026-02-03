/**
 *
 *  @file Result.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
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
 * @brief Generic Result<T, E> type for error handling without exceptions.
 *
 * This header defines a lightweight Result class template inspired by Rust's
 * Result. A Result represents either a success value (Ok) of type `T`, or an
 * error (Err) of type `E`.
 *
 * This design enables deterministic, low-overhead error propagation without
 * relying on C++ exceptions.
 *
 * @tparam T Success type.
 * @tparam E Error type (defaults to std::string).
 *
 * @code
 * #include <vix/utils/Result.hpp>
 * #include <iostream>
 *
 * vix::utils::Result<int> divide(int a, int b)
 * {
 *   if (b == 0)
 *     return vix::utils::Result<int>::Err("division by zero");
 *   return vix::utils::Result<int>::Ok(a / b);
 * }
 *
 * int main()
 * {
 *   auto res = divide(10, 2);
 *   if (res.is_ok())
 *     std::cout << "Result: " << res.value() << "\n";
 *   else
 *     std::cerr << "Error: " << res.error() << "\n";
 * }
 * @endcode
 */

namespace vix::utils
{
  /**
   * @brief Tag type used to construct a successful result.
   */
  struct OkTag
  {
    explicit OkTag() = default;
  };

  /**
   * @brief Tag type used to construct an error result.
   */
  struct ErrTag
  {
    explicit ErrTag() = default;
  };

  /**
   * @brief Constant tag for Ok construction.
   */
  inline constexpr OkTag OkTag_v{};

  /**
   * @brief Constant tag for Err construction.
   */
  inline constexpr ErrTag ErrTag_v{};

  /**
   * @class Result
   * @brief Represents either a success (Ok) containing T or a failure (Err) containing E.
   *
   * Storage:
   * - Uses a union to store either the value `T` or the error `E`.
   * - No dynamic allocation is performed by Result itself.
   *
   * Semantics:
   * - Copyable and movable (if T/E support it).
   * - Accessors assert if you access the inactive variant.
   *
   * @tparam T Success type.
   * @tparam E Error type (defaults to std::string).
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

    /**
     * @brief Construct an Ok result from a const reference.
     */
    explicit Result(OkTag, const T &v) : ok_(true), val_(v) {}

    /**
     * @brief Construct an Ok result from an rvalue.
     */
    explicit Result(OkTag, T &&v) : ok_(true), val_(std::move(v)) {}

    /**
     * @brief Construct an Err result from a const reference.
     */
    explicit Result(ErrTag, const E &e) : ok_(false), err_(e) {}

    /**
     * @brief Construct an Err result from an rvalue.
     */
    explicit Result(ErrTag, E &&e) : ok_(false), err_(std::move(e)) {}

  public:
    /**
     * @brief Create a success result (Ok) by value.
     *
     * @param v Success value.
     * @return Ok result containing the value.
     */
    static Result Ok(T v) { return Result(OkTag_v, std::move(v)); }

    /**
     * @brief Create an error result (Err) by value.
     *
     * @param e Error value.
     * @return Err result containing the error.
     */
    static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

    /**
     * @brief Copy constructor.
     *
     * Copies either the success value or the error value depending on the state.
     *
     * @param o Source result.
     */
    Result(const Result &o) : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(val_)) T(o.val_);
      else
        ::new (std::addressof(err_)) E(o.err_);
    }

    /**
     * @brief Move constructor.
     *
     * Moves either the success value or the error value depending on the state.
     *
     * @param o Source result.
     */
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

    /**
     * @brief Destructor.
     *
     * Destroys the active union member.
     */
    ~Result()
    {
      if (ok_)
        val_.~T();
      else
        err_.~E();
    }

    /**
     * @brief Copy assignment.
     *
     * Assigns the underlying value/error, properly switching active union
     * member when needed.
     *
     * @param o Source result.
     * @return *this
     */
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

    /**
     * @brief Move assignment.
     *
     * Moves the underlying value/error, properly switching active union member
     * when needed.
     *
     * @param o Source result.
     * @return *this
     */
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

    /**
     * @brief Check whether this result is Ok.
     *
     * @return True if Ok, false if Err.
     */
    bool is_ok() const noexcept { return ok_; }

    /**
     * @brief Check whether this result is Err.
     *
     * @return True if Err, false if Ok.
     */
    bool is_err() const noexcept { return !ok_; }

    /**
     * @brief Access the success value (const).
     *
     * @return Const reference to the success value.
     * @warning Asserts if the result is Err.
     */
    const T &value() const
    {
      assert(ok_);
      return val_;
    }

    /**
     * @brief Access the success value (mutable).
     *
     * @return Reference to the success value.
     * @warning Asserts if the result is Err.
     */
    T &value()
    {
      assert(ok_);
      return val_;
    }

    /**
     * @brief Access the error value (const).
     *
     * @return Const reference to the error value.
     * @warning Asserts if the result is Ok.
     */
    const E &error() const
    {
      assert(!ok_);
      return err_;
    }

    /**
     * @brief Access the error value (mutable).
     *
     * @return Reference to the error value.
     * @warning Asserts if the result is Ok.
     */
    E &error()
    {
      assert(!ok_);
      return err_;
    }

    /**
     * @brief Construct an Ok from a const reference.
     *
     * @param v Success value.
     * @return Ok result.
     */
    static Result FromOk(const T &v) { return Result(OkTag_v, v); }

    /**
     * @brief Construct an Ok from an rvalue.
     *
     * @param v Success value.
     * @return Ok result.
     */
    static Result FromOk(T &&v) { return Result(OkTag_v, std::move(v)); }

    /**
     * @brief Construct an Err from a const reference.
     *
     * @param e Error value.
     * @return Err result.
     */
    static Result FromErr(const E &e) { return Result(ErrTag_v, e); }

    /**
     * @brief Construct an Err from an rvalue.
     *
     * @param e Error value.
     * @return Err result.
     */
    static Result FromErr(E &&e) { return Result(ErrTag_v, std::move(e)); }

    /**
     * @brief Default construction is disabled.
     *
     * A Result must be explicitly created as Ok or Err.
     */
    Result() = delete;
  };

  /**
   * @class Result<void, E>
   * @brief Specialization for operations that return void on success.
   *
   * On success, no value is stored. On error, an `E` is stored.
   *
   * @tparam E Error type.
   *
   * @code
   * vix::utils::Result<void> write_file(const std::string &path)
   * {
   *   if (!has_permission(path))
   *     return vix::utils::Result<void>::Err("Permission denied");
   *   return vix::utils::Result<void>::Ok();
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

    /**
     * @brief Construct an Ok result (no value stored).
     */
    explicit Result(OkTag) : ok_(true), dummy_(0) {}

    /**
     * @brief Construct an Err result from a const reference.
     */
    explicit Result(ErrTag, const E &e) : ok_(false), err_(e) {}

    /**
     * @brief Construct an Err result from an rvalue.
     */
    explicit Result(ErrTag, E &&e) : ok_(false), err_(std::move(e)) {}

  public:
    /**
     * @brief Create an Ok result.
     *
     * @return Ok result.
     */
    static Result Ok() { return Result(OkTag_v); }

    /**
     * @brief Create an Err result by value.
     *
     * @param e Error value.
     * @return Err result.
     */
    static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

    /**
     * @brief Copy constructor.
     *
     * @param o Source result.
     */
    Result(const Result &o) : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(dummy_)) char(0);
      else
        ::new (std::addressof(err_)) E(o.err_);
    }

    /**
     * @brief Move constructor.
     *
     * @param o Source result.
     */
    Result(Result &&o) noexcept(std::is_nothrow_move_constructible_v<E>)
        : ok_(o.ok_)
    {
      if (ok_)
        ::new (std::addressof(dummy_)) char(0);
      else
        ::new (std::addressof(err_)) E(std::move(o.err_));
    }

    /**
     * @brief Destructor.
     *
     * Destroys the error value if present.
     */
    ~Result()
    {
      if (!ok_)
        err_.~E();
    }

    /**
     * @brief Copy assignment.
     *
     * @param o Source result.
     * @return *this
     */
    Result &operator=(const Result &o)
    {
      if (this == &o)
        return *this;

      if (ok_ && o.ok_)
      {
        // both ok: nothing to do
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

    /**
     * @brief Move assignment.
     *
     * @param o Source result.
     * @return *this
     */
    Result &operator=(Result &&o) noexcept(
        std::is_nothrow_move_assignable_v<E> &&
        std::is_nothrow_move_constructible_v<E>)
    {
      if (this == &o)
        return *this;

      if (ok_ && o.ok_)
      {
        // both ok: nothing to do
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

    /**
     * @brief Check whether this result is Ok.
     *
     * @return True if Ok, false if Err.
     */
    bool is_ok() const noexcept { return ok_; }

    /**
     * @brief Check whether this result is Err.
     *
     * @return True if Err, false if Ok.
     */
    bool is_err() const noexcept { return !ok_; }

    /**
     * @brief Access the error value (const).
     *
     * @return Const reference to the error value.
     * @warning Asserts if the result is Ok.
     */
    const E &error() const
    {
      assert(!ok_);
      return err_;
    }

    /**
     * @brief Access the error value (mutable).
     *
     * @return Reference to the error value.
     * @warning Asserts if the result is Ok.
     */
    E &error()
    {
      assert(!ok_);
      return err_;
    }

    /**
     * @brief Default construction is disabled.
     *
     * A Result must be explicitly created as Ok or Err.
     */
    Result() = delete;
  };

} // namespace vix::utils

#endif // VIX_UTILS_RESULT_HPP
