#ifndef CHANNELS_ARC_H
#define CHANNELS_ARC_H

#include <cassert>
#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>

namespace chnl {
namespace detail {

template <typename T>
class Manager;

} // namespace detail

template <typename T>
class Arc {
public:
    T& operator*() const {
        assert(ptr_);

        return *ptr_->get();
    }

    T* operator->() const {
        assert(ptr_);

        return ptr_->get();
    }

private:
    std::atomic<detail::Manager<T>*> ptr_;
};

namespace detail {

template <typename T>
class Manager {
public:
    constexpr Manager() noexcept = default;

    Manager(const Manager &other) = delete;

    Manager(Manager &&other) = delete;

    Manager& operator=(const Manager &other) = delete;

    Manager& operator=(Manager &&other) = delete;

    virtual ~Manager() = default;

    T* get() const noexcept {
        return do_get();
    }

    long increment(std::memory_order order = std::memory_order_seq_cst) noexcept {
        return use_count_.fetch_add(1, order);
    }

    long decrement(std::memory_order order = std::memory_order_seq_cst) noexcept {
        return use_count_.fetch_sub(1, order);
    }

    long increment_weak(std::memory_order order = std::memory_order_seq_cst) noexcept {
        return weak_count_.fetch_add(1, order);
    }

    long decrement_weak(std::memory_order order = std::memory_order_seq_cst) noexcept {
        return weak_count_.fetch_sub(1, order);
    }

private:
    virtual T* do_get() const noexcept;

    std::atomic<long> use_count_{ 1 };
    std::atomic<long> weak_count_{ 0 };
};

template <typename T>
class InlineManager : public Manager<T> {
public:
    template <typename ...Ts>
    explicit InlineManager(Ts &&...ts) noexcept(std::is_nothrow_constructible_v<T, Ts...>)
    : obj_{ std::forward<Ts>(ts)... } { }

    template <typename U, typename ...Ts>
    explicit InlineManager(std::initializer_list<U> list, Ts &&...ts)
    noexcept(std::is_nothrow_constructible_v<std::initializer_list<U>&, Ts...>)
    : obj_{ list, std::forward<Ts>(ts)... } { }

    virtual ~InlineManager() = default;

private:
    virtual T* do_get() const noexcept {
        return std::addressof(obj_);
    }

    T obj_;
};

template <typename T, typename D = std::default_delete<T>>
class AdoptManager : public Manager<T> {
public:
    AdoptManager(T *obj_ptr, const D &deleter = D{ })
    noexcept(std::is_nothrow_copy_constructible_v<D>)
    : obj_ptr_{ obj_ptr }, deleter_{ deleter } { }

    virtual ~AdoptManager() {
        deleter_(obj_ptr_);
    }

private:
    virtual T* do_get() const noexcept {
        return obj_ptr_;
    }

    T *obj_ptr_;
    D deleter_;
};

} // namespace detail

} // namespace chnl

#endif
