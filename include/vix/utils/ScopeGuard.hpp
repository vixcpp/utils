/**
 *
 *  @file ScopeGuard.hpp
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
#ifndef VIX_SCOPE_GUARD_HPP
#define VIX_SCOPE_GUARD_HPP

#include <utility>
#include <type_traits>

/**
 * @brief Minimal scope guard (RAII) to run a callback at scope exit.
 *
 * A `ScopeGuard` executes a user-provided callable when the guard goes out of
 * scope, unless explicitly dismissed via dismiss().
 *
 * - RAII: guarantees cleanup even on exceptions or early returns
 * - Movable: transfer ownership safely; non-copyable
 * - Noexcept destructor: destructor never throws
 * - Exception safety: callback exceptions are swallowed at scope exit
 *
 * @code
 * #include <vix/utils/ScopeGuard.hpp>
 *
 * void example()
 * {
 *   bool committed = false;
 *
 *   auto g = vix::utils::make_scope_guard([&] {
 *     if (!committed)
 *     {
 *       rollback(); // runs on scope exit unless dismissed
 *     }
 *   });
 *
 *   do_work();
 *   commit();
 *   committed = true;
 *   g.dismiss(); // prevent rollback()
 * }
 * @endcode
 */

namespace vix::utils
{
  /**
   * @class ScopeGuard
   * @brief Executes a stored callable at scope exit unless dismissed.
   *
   * Constructed from any callable invocable with no arguments. On destruction,
   * if still active, the callable is executed. Any exception thrown by the
   * callable is caught and suppressed (destructor is noexcept).
   *
   * @note The callable is type-erased and heap-allocated internally.
   * @note Thread-safety: do not share the same instance across threads without
   * external synchronization.
   */
  class ScopeGuard
  {
    bool (*invoke_)(void *) noexcept;
    void (*destroy_)(void *) noexcept;
    void *data_;
    bool active_;

  public:
    /**
     * @brief Construct a guard from a callable.
     *
     * The callable is copied/moved into heap storage and will be invoked on
     * destruction unless dismiss() is called or the guard is moved-from.
     *
     * @tparam F Callable type; must be invocable as `f()`.
     * @param f The callable to store.
     *
     * @note Exceptions thrown by `f()` at scope exit are caught and ignored.
     * @note SFINAE prevents accidental construction from another ScopeGuard.
     */
    template <class F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, ScopeGuard>>>
    explicit ScopeGuard(F &&f)
        : invoke_([](void *p) noexcept -> bool
                  {
                    try
                    {
                      (*static_cast<F *>(p))();
                      return true;
                    }
                    catch (...)
                    {
                      return false;
                    } }),
          destroy_([](void *p) noexcept
                   { delete static_cast<F *>(p); }),
          data_(static_cast<void *>(new F(std::forward<F>(f)))), active_(true)
    {
    }

    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;

    /**
     * @brief Move-construct a scope guard.
     *
     * Ownership of the stored callable is transferred. The moved-from guard is
     * deactivated and will not run the callback.
     *
     * @param other Guard to move from.
     */
    ScopeGuard(ScopeGuard &&other) noexcept
        : invoke_(other.invoke_), destroy_(other.destroy_), data_(other.data_), active_(other.active_)
    {
      other.data_ = nullptr;
      other.active_ = false;
    }

    /**
     * @brief Move-assign a scope guard.
     *
     * If this guard currently owns an active callable, it is executed before
     * being destroyed, then ownership is transferred from `other`.
     *
     * @param other Guard to move from.
     * @return Reference to this guard.
     */
    ScopeGuard &operator=(ScopeGuard &&other) noexcept
    {
      if (this != &other)
      {
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

    /**
     * @brief Destructor executes the callback if still active.
     *
     * The destructor never throws. If the callback throws, the exception is
     * swallowed.
     */
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

    /**
     * @brief Disable callback execution at scope exit.
     *
     * After calling this, the destructor will not run the stored callable.
     */
    void dismiss() noexcept { active_ = false; }
  };

  /**
   * @brief Helper function for type deduction.
   *
   * Creates a ScopeGuard from a callable.
   *
   * @tparam F Callable type.
   * @param f Callable to run on scope exit.
   * @return A ScopeGuard holding `f`.
   *
   * @code
   * auto g = vix::utils::make_scope_guard([&] { close(fd); });
   * @endcode
   */
  template <class F>
  inline ScopeGuard make_scope_guard(F &&f)
  {
    return ScopeGuard(std::forward<F>(f));
  }

} // namespace vix::utils

#endif // VIX_SCOPE_GUARD_HPP
