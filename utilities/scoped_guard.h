#pragma once

#include <new>
#include <utility>

namespace utilities
{

template<typename Func>
class [[nodiscard]] scoped_guard
{
public:
    template<typename F>
    explicit scoped_guard(F&& func)
        : func_(std::forward<F>(func))
    {
    }

    ~scoped_guard() noexcept(noexcept(std::declval<Func>()())) {
        func_();
    }

    scoped_guard(const scoped_guard&) = delete;
    scoped_guard& operator=(const scoped_guard&) = delete;
    scoped_guard(scoped_guard&&) = delete;
    scoped_guard& operator=(scoped_guard&&) = delete;
    void* operator new(std::size_t) = delete;
    void* operator new(std::size_t, void*) = delete;
    void* operator new(std::size_t, std::align_val_t) = delete;
    void* operator new(std::size_t, const std::nothrow_t&) = delete;
    void* operator new[](std::size_t) = delete;
    void* operator new[](std::size_t, void*) = delete;
    void* operator new[](std::size_t, std::align_val_t) = delete;
    void* operator new[](std::size_t, const std::nothrow_t&) = delete;

private:
    Func func_;
};

template<typename Func>
scoped_guard(Func)->scoped_guard<Func>;

} //namespace utilities
