#include <array>

// TODO: SoA flat map

namespace {
static constexpr initial_capacity = 4;

template <typename Type>
union compressed_small_array {
    std::array<Type, initial_capacity> array;
    Type* pointer = nullptr;

    Type* get(size_t size) {
        return size <= initial_capacity ? array.data() : pointer;
    }
    const Type* get(size_t size) const {
        return size <= initial_capacity ? array.data() : pointer;
    }
};
}

template <typename Type, class Allocator = std::allocator<T>>
class small_vector {
   public:
    using value_type = Type;
    using allocator_type = Allocator;

    Type& operator[](size_t pos) { return data_.get(size)[pos]; }
    Type const& operator[](size_t pos) const { return data_.get(size)[pos]; }

    Type* data() { return data_.get(size_); }
    Type const* data() const { return data_.get(size_); }

    Type& front() { return data_.get(size_)[0]; }
    Type const& front() const { return data_.get(size_)[0]; }

    Type& back() { return data_.get(size_)[size_ - 1]; }
    Type const& back() const { return data_.get(size_)[size_ - 1]; }

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }

    // clear
    // insert
    // emplace
    // erase
    // push_back
    // emplace_back
    // pop_back
    // resize
    // swap

   private:
    compressed_small_array<Type> data_;
    uint16_t size_ = 0;
    uint16_t capacity_ = initial_capacity;
};

int main() {}
