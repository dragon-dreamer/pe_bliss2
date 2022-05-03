#pragma once

#include <new>
#include <utility>

namespace utilities
{

namespace impl
{
template<typename Func>
class [[nodiscard]] scoped_guard_base
{
public:
    template<typename F>
    explicit scoped_guard_base(F&& func)
        : func_(std::forward<F>(func))
    {
    }

    scoped_guard_base(const scoped_guard_base&) = delete;
    scoped_guard_base& operator=(const scoped_guard_base&) = delete;
    scoped_guard_base(scoped_guard_base&&) = delete;
    scoped_guard_base& operator=(scoped_guard_base&&) = delete;
    void* operator new(std::size_t) = delete;
    void* operator new(std::size_t, void*) = delete;
    void* operator new(std::size_t, std::align_val_t) = delete;
    void* operator new(std::size_t, const std::nothrow_t&) = delete;
    void* operator new[](std::size_t) = delete;
    void* operator new[](std::size_t, void*) = delete;
    void* operator new[](std::size_t, std::align_val_t) = delete;
    void* operator new[](std::size_t, const std::nothrow_t&) = delete;

protected:
    Func func_;
};
} //namespace impl

template<typename Func>
class [[nodiscard]] scoped_guard final
    : public impl::scoped_guard_base<Func>
{
public:
    using impl::scoped_guard_base<Func>::scoped_guard_base;
    using impl::scoped_guard_base<Func>::operator=;
    using impl::scoped_guard_base<Func>::operator new;
    using impl::scoped_guard_base<Func>::operator new[];

    ~scoped_guard() noexcept(noexcept(std::declval<Func>()())) {
        this->func_();
    }
};

template<typename Func>
class [[nodiscard]] releasable_scoped_guard final
    : public impl::scoped_guard_base<Func>
{
public:
    using impl::scoped_guard_base<Func>::scoped_guard_base;
    using impl::scoped_guard_base<Func>::operator=;
    using impl::scoped_guard_base<Func>::operator new;
    using impl::scoped_guard_base<Func>::operator new[];

    ~releasable_scoped_guard()
        noexcept(noexcept(std::declval<Func>()())) {
        if (!released_)
            this->func_();
    }

    void release() noexcept
    {
        released_ = true;
    }

private:
    bool released_ = false;
};

template<typename Func>
scoped_guard(Func)->scoped_guard<Func>;
template<typename Func>
releasable_scoped_guard(Func)->releasable_scoped_guard<Func>;

} //namespace utilities
