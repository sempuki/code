#ifndef SRC_CORE_BUFFER_HPP_
#define SRC_CORE_BUFFER_HPP_

#include "core/common.hpp"

namespace core {

template <typename Type>
struct AliasBuffer {
    Type *base = nullptr;
    size_t size = 0;

    AliasBuffer(Type *base, size_t size) : base{base}, size{size} {}

    template <typename T>
    AliasBuffer(const AliasBuffer<T> &copy)
        : base{copy.base}, size{copy.size} {}

    template <typename T>
    AliasBuffer &operator=(const AliasBuffer<T> &that) {
        base = that.base;
        size = that.size;
        return *this;
    }

    explicit operator bool() const { return base; }
};

template <typename Type>
struct UniqueBuffer {
    std::unique_ptr<Type, std::function<void(Type *)>> base;
    size_t size = 0;

    UniqueBuffer(Type *base, size_t size) : base{base}, size{size} {}

    template <typename D>
    UniqueBuffer(Type *base, D &&deleter, size_t size)
        : base{base, deleter}, size{size} {}

    template <typename T>
    explicit UniqueBuffer(const AliasBuffer<T> &copy)
        : base{copy.base}, size{copy.size} {}

    template <typename T>
    UniqueBuffer(UniqueBuffer<T> &&temp)
        : base{std::move(temp.base)}, size{temp.size} {}

    template <typename T, typename D>
    UniqueBuffer(std::unique_ptr<T, D> &&temp, size_t size)
        : base{std::move(temp)}, size{size} {}

    template <typename T>
    UniqueBuffer &operator=(const AliasBuffer<T> &that) {
        base = that.base;
        size = that.size;
        return *this;
    }

    template <typename T>
    UniqueBuffer &operator=(UniqueBuffer<T> &&temp) {
        base = std::move(temp.base);
        size = temp.size;
        return *this;
    }

    explicit operator bool() const { return base; }
};

template <typename Type>
struct SharedBuffer {
    std::shared_ptr<Type> base;
    size_t size = 0;

    SharedBuffer(Type *base, size_t size) : base{base}, size{size} {}

    template <typename D>
    SharedBuffer(Type *base, D &&deleter, size_t size)
        : base{base, deleter}, size{size} {}

    template <typename T>
    explicit SharedBuffer(const AliasBuffer<T> &copy)
        : base{copy.base}, size{copy.size} {}

    template <typename T>
    explicit SharedBuffer(UniqueBuffer<T> &&temp)
        : base{std::move(temp.base)}, size{temp.size} {}

    template <typename T>
    SharedBuffer(const SharedBuffer<T> &copy)
        : base{copy.base}, size{copy.size} {}

    template <typename T, typename D>
    SharedBuffer(std::unique_ptr<T, D> &&temp, size_t size)
        : base{std::move(temp)}, size{size} {}

    template <typename T>
    SharedBuffer(const std::shared_ptr<T> &temp, size_t size)
        : base{temp}, size{size} {}

    template <typename T>
    SharedBuffer &operator=(const AliasBuffer<T> &that) {
        base = that.base;
        size = that.size;
        return *this;
    }

    template <typename T>
    SharedBuffer &operator=(UniqueBuffer<T> &&temp) {
        base = std::move(temp.base);
        size = temp.size;
        return *this;
    }

    explicit operator bool() const { return base; }
};

template <typename Type>
Type *begin(const AliasBuffer<Type> &buffer) {
    return buffer.base;
}

template <typename Type>
Type *end(const AliasBuffer<Type> &buffer) {
    return buffer.base + buffer.size;
}

template <typename Type>
Type *begin(const UniqueBuffer<Type> &buffer) {
    return buffer.base.get();
}

template <typename Type>
Type *end(const UniqueBuffer<Type> &buffer) {
    return buffer.base.get() + buffer.size;
}

template <typename Type>
Type *begin(const SharedBuffer<Type> &buffer) {
    return buffer.base.get();
}

template <typename Type>
Type *end(const SharedBuffer<Type> &buffer) {
    return buffer.base.get() + buffer.size;
}
/*
template <typename Type>
Buffer<Type> resize(const Buffer<Type> &buffer, size_t size) {
    return {buffer.base, size};
}

template <typename Type>
Buffer<Type> grow(const Buffer<Type> &buffer, int size) {
    return {buffer.base, buffer.size + size};
}

template <typename Type>
Buffer<Type> slide(const Buffer<Type> &buffer, int size) {
    return {buffer.base + size, buffer.size};
}

template <typename Type>
Buffer<Type> advance(const Buffer<Type> &buffer, int size) {
    return {buffer.base + size, buffer.size > size ? buffer.size - size : 0};
}

template <typename ToType, typename FromType>
Buffer<ToType> reinterpret_buffer(const Buffer<FromType> &buffer) {
    return {reinterpret_cast<ToType *>(buffer.base),
            (buffer.size * sizeof(FromType)) / sizeof(ToType)};
}

template <typename ToType, typename FromType>
Buffer<ToType> reinterpret_buffer_safe(const Buffer<FromType> &buffer) {
    Buffer<ToType> result;

    auto size = (buffer.size * sizeof(FromType)) / sizeof(ToType);
    auto truncated = (size * sizeof(ToType)) < (buffer.size * sizeof(FromType));
    if (!truncated) {
        result = {reinterpret_cast<ToType *>(buffer.base), size};
    }

    assert(truncated == false && "Truncated buffer");
    return result;
}

template <typename Type>
Buffer<Type> align_size(const Buffer<Type> &buffer, int alignment) {
    return {buffer.base, buffer.size - (buffer.size % alignment)};
}
*/
template <template <typename> class SrcBuffer, typename SrcType,
          template <typename> class DstBuffer, typename DstType>
void copy(const SrcBuffer<SrcType> &source,
          const DstBuffer<DstType> &destination) {
    static_assert(sizeof(SrcType) == sizeof(DstType), "Type size mis-match");
    count = std::min(destination.size, source.size);
    std::copy_n(source.base, count, destination.base);
}

template <template <typename> class SrcBuffer, typename SrcType,
          template <typename> class DstBuffer, typename DstType>
void copy(const SrcBuffer<SrcType> &source,
          const DstBuffer<DstType> &destination, size_t bytes) {
    static_assert(sizeof(SrcType) == sizeof(DstType), "Type size mis-match");
    count = std::min({count, destination.size, source.size});
    std::copy_n(source.base, count, destination.base);
}

template <typename Type>
SharedBuffer<Type> share(UniqueBuffer<Type> &&buffer) {
    return {std::move(buffer.base), buffer.size};
}

template <typename Type>
AliasBuffer<Type> alias(const UniqueBuffer<Type> &buffer) {
    return {buffer.base.get(), buffer.size};
}

template <typename Type>
AliasBuffer<Type> alias(const SharedBuffer<Type> &buffer) {
    return {buffer.base.get(), buffer.size};
}

using ByteBuffer = AliasBuffer<uint8_t>;
using ConstByteBuffer = AliasBuffer<const uint8_t>;
using UniqueByteBuffer = UniqueBuffer<uint8_t>;
using SharedByteBuffer = SharedBuffer<uint8_t>;

inline ByteBuffer bytebuffer(void *pointer, size_t bytes) {
    return {static_cast<uint8_t *>(pointer), bytes};
}

inline ConstByteBuffer bytebuffer(const void *pointer, size_t bytes) {
    return {static_cast<const uint8_t *>(pointer), bytes};
}

const int kMessageBufferSize = 1500;

template <typename Type>
std::string describe(AliasBuffer<Type> buffer) {
    return std::to_string(buffer.size);
}

template <typename Type>
std::ostream &operator<<(std::ostream &out, AliasBuffer<Type> buffer) {
    out << describe(buffer);
    return out;
}

}  // namespace core

#endif
