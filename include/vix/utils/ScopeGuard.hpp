#ifndef VIX_SCOPE_GUARD_HPP
#define VIX_SCOPE_GUARD_HPP

#include <utility>
#include <type_traits>

/**
 * @file VIX_SCOPE_GUARD_HPP
 * @brief Minimal scope guard (RAII) to run a callback at scope exit.
 *
 * `ScopeGuard::ScopeGuard` executes a user-provided callable when the guard
 * goes out of scope, unless explicitly dismissed via `dismiss()`.
 *
 * - **RAII**: guarantees cleanup even on exceptions or early returns
 * - **Movable**: transfer ownership safely; non-copyable
 * - **Noexcept dtor**: destructor never throws
 * - **Exception-safety**: callback exceptions are swallowed inside the guard
 *   (they are NOT propagated at scope exit)
 *
 * ### Example
 * @code
 * using ScopeGuard::make_scope_guard;
 *
 * void example() {
 *   bool committed = false;
 *   auto g = make_scope_guard([&]{
 *     if (!committed) rollback();  // runs on scope exit unless dismissed
 *   });
 *
 *   do_work();
 *   committed = true;
 *   commit();
 *   g.dismiss(); // prevent rollback()
 * } // if not dismissed, callback executes here
 * @endcode
 */

namespace vix::utils
{
  /**
   * @class ScopeGuard
   * @brief Executes a stored callable at scope exit unless dismissed.
   *
   * The guard is constructed from any callable `F` invocable with no arguments.
   * On destruction, if still active, the callable is executed. Any exception
   * thrown by the callable is caught and suppressed (destructor is `noexcept`).
   *
   * @note The callable is type-erased and heap-allocated internally.
   * @note Thread-safety: the guard is not thread-safe by itself; avoid sharing
   *       a single instance across threads without external synchronization.
   */
  class ScopeGuard
  {
    /// Function pointer that invokes the stored callable. Returns true if it ran.
    bool (*invoke_)(void *) noexcept;
    /// Function pointer that destroys the stored callable.
    void (*destroy_)(void *) noexcept;
    /// Opaque pointer to the stored callable (heap-allocated).
    void *data_;
    /// Whether the guard is active (not dismissed).
    bool active_;

  public:
    /**
     * @brief Construct a guard from a callable `F` (non-deducing on self).
     *
     * The callable is copied/moved into heap storage and will be invoked on
     * destruction unless `dismiss()` is called or the guard is moved-from.
     *
     * @tparam F Callable type; must be invocable as `(*F)()`.
     * @param f The callable to store.
     *
     * @note Exceptions thrown by `f()` at scope exit are **caught and ignored**.
     * @note SFINAE prevents accidental copy-constructing from another ScopeGuard.
     */
    template <class F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, ScopeGuard>>>
    explicit ScopeGuard(F &&f)
        : invoke_([](void *p) noexcept -> bool
                  {
                  try {
                      (*static_cast<F*>(p))();
                      return true;
                  } catch (...) {
                      // Do not propagate exceptions from destructors
                      return false;
                  } }),
          destroy_([](void *p) noexcept
                   { delete static_cast<F *>(p); }),
          data_(static_cast<void *>(new F(std::forward<F>(f)))), active_(true)
    {
    }

    /// Non-copyable (unique ownership).
    ScopeGuard(const ScopeGuard &) = delete;
    /// Non-copyable (unique ownership).
    ScopeGuard &operator=(const ScopeGuard &) = delete;

    /**
     * @brief Move constructor: transfers ownership and disarms the source.
     */
    ScopeGuard(ScopeGuard &&other) noexcept
        : invoke_(other.invoke_), destroy_(other.destroy_), data_(other.data_), active_(other.active_)
    {
      other.data_ = nullptr;
      other.active_ = false;
    }

    /**
     * @brief Move assignment: runs & destroys current callable if still active,
     *        then transfers ownership from `other` and disarms it.
     */
    ScopeGuard &operator=(ScopeGuard &&other) noexcept
    {
      if (this != &other)
      {
        // Execute and free the old content if still active
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
     * @brief Destructor (noexcept): invokes the callable if active, then destroys it.
     *
     * Any exception thrown by the callable is caught and ignored to respect
     * the `noexcept` destructor guarantee.
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
     * @brief Disarm the guard: prevent the callable from running at scope exit.
     *
     * Safe to call multiple times; subsequent calls are no-ops.
     */
    void dismiss() noexcept { active_ = false; }
  };

  /**
   * @brief Helper for type deduction (CTAD alternative).
   *
   * @tparam F Callable type.
   * @param f Callable to run on scope exit.
   * @return A `ScopeGuard` holding `f`.
   *
   * @code
   * auto g = ScopeGuard::make_scope_guard([&]{ close(fd); });
   * @endcode
   */
  template <class F>
  inline ScopeGuard make_scope_guard(F &&f)
  {
    return ScopeGuard(std::forward<F>(f));
  }

} // namespace vix::utils

#endif // VIX_SCOPE_GUARD_HPP
