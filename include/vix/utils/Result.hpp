#ifndef VIX_RESULT_HPP
#define VIX_RESULT_HPP

#include <string>
#include <utility>
#include <type_traits>
#include <new> // std::launder (C++17),
#include <cassert>

namespace Vix::utils
{
    // ---------------- Tags ----------------
    struct OkTag
    {
        explicit OkTag() = default;
    };
    struct ErrTag
    {
        explicit ErrTag() = default;
    };
    inline constexpr OkTag OkTag_v{};
    inline constexpr ErrTag ErrTag_v{};

    // =========================================================
    // Result<T, E>
    // =========================================================
    template <typename T, typename E = std::string>
    class Result
    {
        bool ok_;
        union
        {
            T val_;
            E err_;
        };

        // --- ctors internes : activent le bon membre du union ---
        explicit Result(OkTag, const T &v) : ok_(true), val_(v) {}
        explicit Result(OkTag, T &&v) : ok_(true), val_(std::move(v)) {}
        explicit Result(ErrTag, const E &e) : ok_(false), err_(e) {}
        explicit Result(ErrTag, E &&e) : ok_(false), err_(std::move(e)) {}

    public:
        // --- factories ---
        static Result Ok(T v) { return Result(OkTag_v, std::move(v)); }
        static Result Err(E e) { return Result(ErrTag_v, std::move(e)); }

        // --- copy / move ctors ---
        Result(const Result &o) : ok_(o.ok_)
        {
            if (ok_)
                ::new (std::addressof(val_)) T(o.val_);
            else
                ::new (std::addressof(err_)) E(o.err_);
        }

        Result(Result &&o) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                    std::is_nothrow_move_constructible_v<E>)
            : ok_(o.ok_)
        {
            if (ok_)
                ::new (std::addressof(val_)) T(std::move(o.val_));
            else
                ::new (std::addressof(err_)) E(std::move(o.err_));
        }

        // --- dtor ---
        ~Result()
        {
            if (ok_)
                val_.~T();
            else
                err_.~E();
        }

        // --- copy/move assignment (fortement exception-safe) ---
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
                // val_ -> err_
                val_.~T();
                ::new (std::addressof(err_)) E(o.err_);
                ok_ = false;
            }
            else
            { // !ok_ && o.ok_
                err_.~E();
                ::new (std::addressof(val_)) T(o.val_);
                ok_ = true;
            }
            return *this;
        }

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
            { // !ok_ && o.ok_
                err_.~E();
                ::new (std::addressof(val_)) T(std::move(o.val_));
                ok_ = true;
            }
            return *this;
        }

        // --- observers ---
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

        // --- helpers pour construction directe (optionnels) ---
        static Result FromOk(const T &v) { return Result(OkTag_v, v); }
        static Result FromOk(T &&v) { return Result(OkTag_v, std::move(v)); }
        static Result FromErr(const E &e) { return Result(ErrTag_v, e); }
        static Result FromErr(E &&e) { return Result(ErrTag_v, std::move(e)); }

        // pas de ctor par défaut : un Result doit être Ok ou Err
        Result() = delete;
    };

    // =========================================================
    // Result<void, E>
    // =========================================================
    template <typename E>
    class Result<void, E>
    {
        bool ok_;
        union
        {
            char dummy_; // actif quand ok_ == true
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
            { // !ok_ && o.ok_
                err_.~E();
                ::new (std::addressof(dummy_)) char(0);
                ok_ = true;
            }
            return *this;
        }

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
            { // !ok_ && o.ok_
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

} // namespace Vix::utils

#endif // VIX_RESULT_HPP
