#ifndef VIX_SCOPE_GUARD_HPP
#define VIX_SCOPE_GUARD_HPP

#include <utility>

namespace ScopeGuard
{
    class ScopeGuard
    {
        bool active_ = true;
        void (*fn_)(void *);
        void *data_;

    public:
        template <class F>
        explicit ScopeGuard(F &&f) : fn_([](void *p)
                                         { (*static_cast<F *>(p))(); }),
                                     data_(new F(std::forward<F>(f))) {}
        ~ScopeGuard()
        {
            if (active_)
                fn_(data_);
            delete static_cast<void *>(data_);
        }
        void dismiss() { active_ = false; }
    };
}

#endif