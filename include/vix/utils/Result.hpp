#ifndef VIX_RESULT_HPP
#define VIX_RESULT_HPP

#include <string>
#include <utility>

namespace Vix::utils
{

    // ---------Result<T,E> ---------
    template <typename T, typename E = std::string>
    class Result
    {
        bool ok_;
        union
        {
            T val_;
            E err_;
        };

    public:
        static Result Ok(T v)
        {
            Result r;
            r.ok_ = true;
            new (&r.val_) T(std::move(v));
            return r;
        }
        static Result Err(E e)
        {
            Result r;
            r.ok_ = false;
            new (&r.err_) E(std::move(e));
            return r;
        }

        Result(const Result &o)
        {
            ok_ = o.ok_;
            if (ok_)
                new (&val_) T(o.val_);
            else
                new (&err_) E(o.err_);
        }
        Result(Result &&o) noexcept
        {
            ok_ = o.ok_;
            if (ok_)
                new (&val_) T(std::move(o.val_));
            else
                new (&err_) E(std::move(o.err_));
        }
        Result &operator=(Result o)
        {
            this->~Result();
            new (this) Result(std::move(o));
            return *this;
        }
        ~Result()
        {
            if (ok_)
                val_.~T();
            else
                err_.~E();
        }

        bool is_ok() const { return ok_; }
        bool is_err() const { return !ok_; }
        const T &value() const { return val_; }
        T &value() { return val_; }
        const E &error() const { return err_; }
        E &error() { return err_; }

    private:
        Result() : ok_(false), err_() {} // hidden base constructor
    };

    // --------- Specialization: Result<void,E> ---------
    template <typename E>
    class Result<void, E>
    {
        bool ok_;
        union
        {
            char dummy_; // placeholder
            E err_;
        };

    public:
        static Result Ok()
        {
            Result r;
            r.ok_ = true;
            r.dummy_ = 0;
            return r;
        }
        static Result Err(E e)
        {
            Result r;
            r.ok_ = false;
            new (&r.err_) E(std::move(e));
            return r;
        }

        Result(const Result &o)
        {
            ok_ = o.ok_;
            if (ok_)
                dummy_ = 0;
            else
                new (&err_) E(o.err_);
        }
        Result(Result &&o) noexcept
        {
            ok_ = o.ok_;
            if (ok_)
                dummy_ = 0;
            else
                new (&err_) E(std::move(o.err_));
        }
        Result &operator=(Result o)
        {
            this->~Result();
            new (this) Result(std::move(o));
            return *this;
        }
        ~Result()
        {
            if (!ok_)
                err_.~E();
        }

        bool is_ok() const { return ok_; }
        bool is_err() const { return !ok_; }

        // No value() for void
        const E &error() const { return err_; }
        E &error() { return err_; }

    private:
        Result() : ok_(false), dummy_(0) {}
    };

}

#endif
