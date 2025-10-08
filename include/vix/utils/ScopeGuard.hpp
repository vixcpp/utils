#ifndef VIX_SCOPE_GUARD_HPP
#define VIX_SCOPE_GUARD_HPP

#include <utility>
#include <type_traits>

namespace ScopeGuard
{
    class ScopeGuard
    {
        bool (*invoke_)(void *) noexcept;
        void (*destroy_)(void *) noexcept;
        void *data_;
        bool active_;

    public:
        template <class F,
                  typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, ScopeGuard>>>
        explicit ScopeGuard(F &&f)
            : invoke_([](void *p) noexcept -> bool
                      {
                  try {
                      (*static_cast<F*>(p))();
                      return true;
                  } catch (...) {
                      // we are not propagating to the exit of scope
                      return false;
                  } }),
              destroy_([](void *p) noexcept
                       { delete static_cast<F *>(p); }),
              data_(static_cast<void *>(new F(std::forward<F>(f)))), active_(true)
        {
        }

        ScopeGuard(const ScopeGuard &) = delete;
        ScopeGuard &operator=(const ScopeGuard &) = delete;

        // --- movable ---
        ScopeGuard(ScopeGuard &&other) noexcept
            : invoke_(other.invoke_), destroy_(other.destroy_), data_(other.data_), active_(other.active_)
        {
            other.data_ = nullptr;
            other.active_ = false;
        }
        ScopeGuard &operator=(ScopeGuard &&other) noexcept
        {
            if (this != &other)
            {
                // execute and free the old content if still active
                if (data_)
                {
                    if (active_ && invoke_)
                        (void)invoke_(data_);
                    if (destroy_)
                        destroy_(data_);
                }
                invoke_ = other.invoke_;
                destroy_ = other.destroy_;
                data_ = other.data_;
                active_ = other.active_;
                other.data_ = nullptr;
                other.active_ = false;
            }
            return *this;
        }

        // --- dtor: noexcept and without UB ---
        ~ScopeGuard() noexcept
        {
            if (data_)
            {
                if (active_ && invoke_)
                    (void)invoke_(data_);
                if (destroy_)
                    destroy_(data_);
            }
        }

        // --- disarms execution on exit ---
        void dismiss() noexcept { active_ = false; }
    };

    // helper for type deduction
    template <class F>
    inline ScopeGuard make_scope_guard(F &&f)
    {
        return ScopeGuard(std::forward<F>(f));
    }

} // namespace ScopeGuard

#endif // VIX_SCOPE_GUARD_HPP
