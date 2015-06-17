#include <iostream>
#include <cassert>
#include <memory>
#include <mutex>

/* Orca C++ Challenge
 * - refcount
 * - optional deleter
 * - optional null checking
 * - optional control block allocator
 * - optional thread safety
 */

struct Empty {};

struct NullCheckPolicy {
  template <typename Type>
  static void check(Type *) {}
};

struct NullThreadPolicy {
  using Mutex = Empty;
  using Lock = Empty;
};

struct AssertCheckPolicy {
  template <typename Type>
  static void check(Type *ptr) {
    assert(ptr);
  }
};

struct MutexThreadPolicy {
  using Mutex = std::mutex;
  using Lock = std::lock_guard<Mutex>;
};

template <typename Type,
  typename Allocator = std::allocator<Type>,
  typename CheckPolicy = NullCheckPolicy,
  typename ThreadPolicy = NullThreadPolicy>
class smart_ptr {
 private:
  struct control {
    uint_fast16_t count = 1;
    mutable typename ThreadPolicy::Mutex mutex;
  };

  using ControlAllocator =
    typename std::allocator_traits<Allocator>::template rebind_alloc<control>;

  static control *construct(Allocator &alloc) {
    ControlAllocator ctrlloc {alloc};
    auto ctrl = std::allocator_traits<ControlAllocator>::allocate(ctrlloc, 1);
    std::allocator_traits<ControlAllocator>::construct(ctrlloc, ctrl);
    return ctrl;
  }

  static void destroy(Allocator &alloc, control *ctrl) {
    ControlAllocator ctrlloc {alloc};
    std::allocator_traits<ControlAllocator>::destroy(ctrlloc, ctrl);
    std::allocator_traits<ControlAllocator>::deallocate(ctrlloc, ctrl, 1);
  }

  static void destroy(Allocator &alloc, Type *ptr) {
    std::allocator_traits<Allocator>::destroy(alloc, ptr);
    std::allocator_traits<Allocator>::deallocate(alloc, ptr, 1);
  }


 public:
  smart_ptr(Type *ptr, std::function<void()> ondelete = {}) :
    ptr_ {ptr}, ondelete_ {ondelete} {
    ctrl_ = construct(alloc_);
  }

  smart_ptr(Type *ptr, Allocator &alloc, std::function<void()> ondelete = {}) :
    ptr_ {ptr}, alloc_ {alloc}, ondelete_ {ondelete} {
    ctrl_ = construct(alloc_);
  }

 public:
  smart_ptr(const smart_ptr &that) {
    typename ThreadPolicy::Lock _{that.ctrl_->mutex};
    ptr_ = that.ptr_;
    ctrl_ = that.ctrl_;
    ctrl_->count++;
    ondelete_ = that.ondelete_;
  }

  smart_ptr &operator=(const smart_ptr &that) {
    typename ThreadPolicy::Lock _{that.ctrl_->mutex};
    release();
    ptr_ = that.ptr_;
    ctrl_ = that.ctrl_;
    ctrl_->count++;
    ondelete_ = that.ondelete_;
    return *this;
  }

 public:
  ~smart_ptr() {
    release();
  }

  void release() {
    if (ctrl_) {
      typename ThreadPolicy::Lock _{ctrl_->mutex};
      if (--ctrl_->count == 0) {
        destroy(alloc_, ctrl_), ctrl_ = nullptr;
        destroy(alloc_, ptr_), ptr_ = nullptr;
        if (ondelete_) {
          ondelete_();
        }
      }
    }
  }

 public:
  Type &operator*() const {
    CheckPolicy::check(ptr_);
    return *ptr_;
  }

  Type *operator->() const {
    CheckPolicy::check(ptr_);
    return ptr_;
  }

 private:
  Type *ptr_ = nullptr;
  control *ctrl_ = nullptr;

 private:
  Allocator alloc_;
  std::function<void()> ondelete_;
};

struct Test {
  Test() {
    std::cout << "constructed" << std::endl;
  }
  ~Test() {
    std::cout << "destroyed" << std::endl;
  }
  void foo() {
    std::cout << "hello" << std::endl;
  }
};

int main() {
  smart_ptr<Test, std::allocator<Test>> a {new Test, []{
    std::cout << "on delete" << std::endl;
  }};
  smart_ptr<Test, std::allocator<Test>> b {a};

  a.release();
  b->foo();
  b.release();

  std::cout << "the end" << std::endl;
}
