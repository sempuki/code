#include <cstddef>
#include <utility>

template <typename T>
class unique_ptr final {
   public:
    ~unique_ptr() { delete ptr_; }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr(unique_ptr&& that) { std::swap(ptr_, that.ptr_); }
    unique_ptr& operator=(unique_ptr&& that) {
        if (this != &that) {
            std::swap(ptr_, that.ptr_);
        }
        return *this;
    }

    bool operator==(const unique_ptr& that) const { return ptr_ == that.ptr_; }
    bool operator!=(const unique_ptr& that) const { return ptr_ != that.ptr_; }

    bool operator==(std::nullptr_t) const { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr_ != nullptr; }

    template <typename U = T*>
    explicit unique_ptr(U p = nullptr) : ptr_{p} {}

    template <typename U = T*>
    void reset(U p = nullptr) {
        *this = unique_ptr<T>{p};
    }

    T* release() {
        T* released = nullptr;
        std::swap(ptr_, released);
        return released;
    }

    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }

   private:
    T* ptr_;
};

