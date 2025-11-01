#ifndef VIX_RESULT_HPP
#define VIX_RESULT_HPP

#include <string>
#include <utility>
#include <type_traits>
#include <new> // std::launder (C++17)
#include <cassert>

/**
 * @file VIX_RESULT_HPP
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
    // ---------------------------------------------------------------------
    // Tags for internal construction (OkTag / ErrTag)
    // ---------------------------------------------------------------------

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

    // =====================================================================
    // Result<T, E> — Generic variant
    // =====================================================================

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

        // --- Private constructors (internal use only) ---
        explicit Result(OkTag, const T &v) : ok_(true), val_(v) {}
        explicit Result(OkTag, T &&v) : ok_(true), val_(std::move(v)) {}
        explicit Result(ErrTag, const E &e) : ok_(false), err_(e) {}
        explicit Result(ErrTag, E &&e) : ok_(false), err_(std::move(e)) {}

    public:
        // -----------------------------------------------------------------
        // Factory methods
        // -----------------------------------------------------------------

        /** @brief Creates a successful result. */
        static Result Ok(T v) { return Result(OkTag_v, std::move(v)); }

        /** @brief Creates an error result. */
        static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

        // -----------------------------------------------------------------
        // Constructors / Destructor
        // -----------------------------------------------------------------

        /** @brief Copy constructor. */
        Result(const Result &o) : ok_(o.ok_)
        {
            if (ok_)
                ::new (std::addressof(val_)) T(o.val_);
            else
                ::new (std::addressof(err_)) E(o.err_);
        }

        /** @brief Move constructor. */
        Result(Result &&o) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                    std::is_nothrow_move_constructible_v<E>)
            : ok_(o.ok_)
        {
            if (ok_)
                ::new (std::addressof(val_)) T(std::move(o.val_));
            else
                ::new (std::addressof(err_)) E(std::move(o.err_));
        }

        /** @brief Destructor. Destroys only the active union member. */
        ~Result()
        {
            if (ok_)
                val_.~T();
            else
                err_.~E();
        }

        // -----------------------------------------------------------------
        // Assignment operators
        // -----------------------------------------------------------------

        /** @brief Copy assignment (strong exception safety). */
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

        /** @brief Move assignment (strong exception safety). */
        Result &operator=(Result &&o) noexcept(std::is_nothrow_move_assignable_v<T> &&
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

        // -----------------------------------------------------------------
        // Observers
        // -----------------------------------------------------------------

        /** @return `true` if this Result holds a success value. */
        bool is_ok() const noexcept { return ok_; }

        /** @return `true` if this Result holds an error. */
        bool is_err() const noexcept { return !ok_; }

        /** @return Reference to the success value. Undefined if not Ok. */
        const T &value() const
        {
            assert(ok_);
            return val_;
        }

        /** @return Mutable reference to the success value. */
        T &value()
        {
            assert(ok_);
            return val_;
        }

        /** @return Reference to the error value. Undefined if Ok. */
        const E &error() const
        {
            assert(!ok_);
            return err_;
        }

        /** @return Mutable reference to the error value. */
        E &error()
        {
            assert(!ok_);
            return err_;
        }

        // -----------------------------------------------------------------
        // Convenience helpers
        // -----------------------------------------------------------------

        /** @brief Creates a Result directly from a success value. */
        static Result FromOk(const T &v) { return Result(OkTag_v, v); }

        /** @brief Creates a Result directly from a moved success value. */
        static Result FromOk(T &&v) { return Result(OkTag_v, std::move(v)); }

        /** @brief Creates a Result directly from an error. */
        static Result FromErr(const E &e) { return Result(ErrTag_v, e); }

        /** @brief Creates a Result directly from a moved error. */
        static Result FromErr(E &&e) { return Result(ErrTag_v, std::move(e)); }

        /** @brief Deleted default constructor: a Result must be Ok or Err. */
        Result() = delete;
    };

    // =====================================================================
    // Result<void, E> — specialization for void success type
    // =====================================================================

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
        /** @brief Creates a successful void result. */
        static Result Ok() { return Result(OkTag_v); }

        /** @brief Creates an error result. */
        static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

        /** @brief Copy constructor. */
        Result(const Result &o) : ok_(o.ok_)
        {
            if (ok_)
                ::new (std::addressof(dummy_)) char(0);
            else
                ::new (std::addressof(err_)) E(o.err_);
        }

        /** @brief Move constructor. */
        Result(Result &&o) noexcept(std::is_nothrow_move_constructible_v<E>)
            : ok_(o.ok_)
        {
            if (ok_)
                ::new (std::addressof(dummy_)) char(0);
            else
                ::new (std::addressof(err_)) E(std::move(o.err_));
        }

        /** @brief Destructor. */
        ~Result()
        {
            if (!ok_)
                err_.~E();
        }

        /** @brief Copy assignment. */
        Result &operator=(const Result &o)
        {
            if (this == &o)
                return *this;

            if (ok_ && o.ok_)
            {
                // nothing
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

        /** @brief Move assignment. */
        Result &operator=(Result &&o) noexcept(std::is_nothrow_move_assignable_v<E> &&
                                               std::is_nothrow_move_constructible_v<E>)
        {
            if (this == &o)
                return *this;

            if (ok_ && o.ok_)
            {
                // nothing
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

        /** @return `true` if this Result represents success. */
        bool is_ok() const noexcept { return ok_; }

        /** @return `true` if this Result represents an error. */
        bool is_err() const noexcept { return !ok_; }

        /** @return Reference to the error value. Undefined if Ok. */
        const E &error() const
        {
            assert(!ok_);
            return err_;
        }

        /** @return Mutable reference to the error value. */
        E &error()
        {
            assert(!ok_);
            return err_;
        }

        /** @brief Deleted default constructor (must be Ok or Err). */
        Result() = delete;
    };

} // namespace vix::utils

#endif // VIX_RESULT_HPP
