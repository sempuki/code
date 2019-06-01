#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <random>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <dlfcn.h>

#include "glog/logging.h"

#include "compat/nvenc/nvEncodeAPI.h"  // from ffmpeg

class Sedentary {
  Sedentary(const Sedentary&) = delete;
  Sedentary& operator=(const Sedentary&) = delete;
  Sedentary(Sedentary&&) = delete;
  Sedentary& operator=(Sedentary&&) = delete;
}

namespace dll {
  class Symbol;

  class DynamicLibrary : Sedentary {
   public:
    explicit DynamicLibrary(const std::string& name)
        : handle_(dlopen(name.c_str(), RTLD_LAZY | RTLD_LOCAL)) {}

    ~DynamicLibrary() {
      if (handle_) {
        dlclose(handle_);
      }
    }

    explicit operator bool() const { return handle_; }
    const char* error() const { return dlerror(); }

   private:
    friend class Symbol;
    void* handle_ = nullptr;
  };

  class Symbol {
   public:
    Symbol(DynamicLibrary& library, const std::string& name)
        : symbol_(dlsym(library.handle_, name.c_str())) {}

    explicit operator bool() const { return symbol_; }
    const char* error() const { return dlerror(); }

   protected:
    void* symbol_ = nullptr;
  };

  template <typename Signature>
  class Function;

  template <typename Return, typename... Arguments>
  class Function<Return(Arguments...)> final : public Symbol {
   public:
    using Symbol::Symbol;

    Return operator()(Arguments... arguments) const {
      return reinterpret_cast<Return (*)(Arguments...)>(this->symbol_)(
          std::forward<Arguments>(arguments)...);
    }
  };
}  // namespace dll

namespace cuda {
inline void check(CUresult e, int line, const char* file) {
  if (e != CUDA_SUCCESS) {
    const char* name = nullptr;
    cuGetErrorName(e, &name);
    LOG(FATAL) << "CUDA driver API error " << name << " at line " << line
               << " in file " << file;
  }
}

inline bool check(cudaError_t e, int line, const char* file) {
  if (e != cudaSuccess) {
    LOG(FATAL) << "CUDA runtime API error " << cudaGetErrorName(e)
               << " at line " << line << " in file " << szFile;
  }
}

#define CHECK_CUDA(expr) cuda::check(expr, __LINE__, __FILE__)

template <typename Type, std::size_t Size>
class ManagedArray : public std::array<Type, Size>, Sedentary {
 public:
  ManagedArray() {
#ifdef CUDA_USE_DRIVER_API
    bool managed = false;
    CHECK_CUDA(cuPointerGetAttribute(&managed, CU_POINTER_ATTRIBUTE_IS_MANAGED,
                                     reinterpret_cast<CUdeviceptr>(this)));
    CHECK_TRUE(managed);
#else
    cudaPointerAttributes attr;
    CHECK_CUDA(cudaPointerGetAttributes(&attr, this));
    CHECK_TRUE(attr.isManaged);
#endif
  }

  void* operator new(size_t size) {
#ifdef CUDA_USE_DRIVER_API
    CUdeviceptr ptr;
    CHECK_CUDA(cuMemAllocManaged(&ptr, size, CU_MEM_ATTACH_GLOBAL));
    return reinterpret_cast<void*>(ptr);
#else
    void* result;
    CHECK_CUDA(cudaMallocManaged(&result, size));
    CHECK_CUDA(cudaDeviceSynchronize());
    return result;
#endif
  }

  void operator delete(void* ptr) {
#ifdef CUDA_USE_DRIVER_API
    CHECK_CUDA(cuMemFree(reinterpret_cast<CUdeviceptr>(ptr)));
#else
    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaFree(ptr));
#endif
  }

  void prefetch_to(int device, cudaStream_t stream = 0) {
    CHECK_CUDA(
        cudaMemPrefetchAsync(this->data(), this->size(), device, stream));
  }

  void sync() { CHECK_CUDA(cudaDeviceSynchronize()); }
};

template <typename Type>
class PitchedDeviceArray, Sedentary {
 public:
  PitchedDeviceArray(size_t width, size_t height)
      : width_{width}, height_{height} {
#ifdef CUDA_USE_DRIVER_API
    CHECK_CUDA(
        cuMemAllocPitch(&memory_, &pitch_, width_ * sizeof(Type), height_, 16));
#else
    CHECK_CUDA(
        cudaMallocPitch(&memory_, &pitch_, width_ * sizeof(Type), height_));
#endif
  }

  ~PitchedDeviceArray() {
#ifdef CUDA_USE_DRIVER_API
    CHECK_CUDA(cuMemFree(memory_));
#else
    CHECK_CUDA(cudaFree(memory_));
    memory_ = nullptr;
#endif
  }

  size_t pitch() const { return pitch_; }
  size_t width() const { return width_; }
  size_t height() const { return height_; }

  Type* data() { return reinterpret_cast<Type*>(memory_); }
  const Type* data() const { return reinterpret_cast<const Type*>(memory_); }

  Type* row(size_t n) {
    return reinterpret_cast<Type*>(reinterpret_cast<uint8_t*>(memory_) +
                                   pitch_ * n);
  }
  const Type* row(size_t n) const {
    return reinterpret_cast<const Type*>(
        reinterpret_cast<const uint8_t*>(memory_) + pitch_ * n);
  }

  template <size_t Size>
  cudaError copy_to(ManagedArray<Type, Size> & dest) {
    const auto width_bytes = width() * sizeof(Type);
    CHECK(width_bytes * height() < Size);
    return cudaMemcpy2D(dest.data(), width_bytes, data(), pitch(), width_bytes,
                        height(), cudaMemcpyDefault);
  }

 private:
#ifdef CUDA_USE_DRIVER_API
  CUdeviceptr memory_;
#else
  void* memory_ = nullptr;
#endif
  size_t pitch_ = 0;
  size_t width_ = 0;
  size_t height_ = 0;
};
}  // namespace cuda

namespace nvenc {
const char* statusstr(NVENCSTATUS status) {
  static const char* nvenc_status_string[] = {
      "success",                                  // NV_ENC_SUCCESS
      "no available encode devices",              // NV_ENC_ERR_NO_ENCODE_DEVICE
      "available devices do not support encode",  // NV_ENC_ERR_UNSUPPORTED_DEVICE
      "invalid encoder device",        // NV_ENC_ERR_INVALID_ENCODERDEVICE
      "invalid device",                // NV_ENC_ERR_INVALID_DEVICE
      "needs reinitialization",        // NV_ENC_ERR_DEVICE_NOT_EXIST
      "invalid pointer",               // NV_ENC_ERR_INVALID_PTR
      "invalid completion event",      // NV_ENC_ERR_INVALID_EVENT
      "invalid parameter",             // NV_ENC_ERR_INVALID_PARAM
      "invalid call",                  // NV_ENC_ERR_INVALID_CALL
      "out of memory",                 // NV_ENC_ERR_OUT_OF_MEMORY
      "encoder not initialized",       // NV_ENC_ERR_ENCODER_NOT_INITIALIZED
      "unsupported parameter",         // NV_ENC_ERR_UNSUPPORTED_PARAM
      "lock busy (try again)",         // NV_ENC_ERR_LOCK_BUSY
      "not enough buffer",             // NV_ENC_ERR_NOT_ENOUGH_BUFFER
      "invalid version",               // NV_ENC_ERR_INVALID_VERSION
      "map (of input buffer) failed",  // NV_ENC_ERR_MAP_FAILED
      "need more input (submit more frames!)",     // NV_ENC_ERR_NEED_MORE_INPUT
      "encoder busy (wait a few ms, call again)",  // NV_ENC_ERR_ENCODER_BUSY
      "event not registered",        // NV_ENC_ERR_EVENT_NOT_REGISTERD
      "unknown error",               // NV_ENC_ERR_GENERIC
      "invalid client key license",  // NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY
      "unimplemented",               // NV_ENC_ERR_UNIMPLEMENTED
      "register resource failed",    // NV_ENC_ERR_RESOURCE_REGISTER_FAILED
      "resource is not registered",  // NV_ENC_ERR_RESOURCE_NOT_REGISTERED
      "resource not mapped",         // NV_ENC_ERR_RESOURCE_NOT_MAPPED
  };
  return nvenc_status_string[status];
}

NV_ENCODE_API_FUNCTION_LIST& api() {
  static NV_ENCODE_API_FUNCTION_LIST api = {NV_ENCODE_API_FUNCTION_LIST_VER};
  return api;
}

class HasStatus {
 public:
  bool has_error() const { return status_ != NV_ENC_SUCCESS; }
  const char* error() const {
    return has_error() ? statusstr(status_) : nullptr;
  }

 protected:
  NVENCSTATUS status_;
};

class Session final : public HasStatus, Sedentary {
 public:
  Session() {
    CHECK(api().nvEncOpenEncodeSessionEx);
    CHECK(api().nvEncDestroyEncoder);

    CUcontext context;
#ifdef CUDA_USE_DRIVER_API
    CUdevice device = 0;
    CHECK_CUDA(cuInit(0));
    CHECK_CUDA(cuDeviceGet(&device, 0));
    CHECK_CUDA(cuCtxCreate(&context, 0, device));
#else
    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cuCtxGetCurrent(&context));
#endif

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER};
    params.device = context;
    params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    params.apiVersion = NVENCAPI_VERSION;

    status_ = api().nvEncOpenEncodeSessionEx(&params, &session_);
    if (has_error()) {
      LOG(ERROR) << "Failed to open encode session: " << error();
    }
  }

  ~Session() {
    status_ = api().nvEncDestroyEncoder(session_);
    if (has_error()) {
      LOG(ERROR) << "Failed to destroy encode session: " << error();
    }
  }

  bool supports(GUID encode, NV_ENC_CAPS caps) {
    NV_ENC_CAPS_PARAM param = {NV_ENC_CAPS_PARAM_VER};
    param.capsToQuery = caps;

    int value = 0;
    status_ = api().nvEncGetEncodeCaps(session_, encode, &param, &value);
    if (has_error()) {
      LOG(ERROR) << "Failed to get capability";
    }

    return value;
  }

  void* handle() const { return session_; }

 private:
  void* session_ = nullptr;
};

struct EncoderInputFormat {
  uint32_t pitch, width, height;
  uint32_t frames_per_second;
  NV_ENC_BUFFER_FORMAT format;
};

struct EncoderOutputFormat {
  GUID codec;
  GUID profile;
  GUID preset;
};

class DeviceFrameBuffer final : public HasStatus, Sedentary {
 public:
  explicit DeviceFrameBuffer(const Session& session, EncoderInputFormat input,
                             void* buffer)
      : session_(session.handle()), buffer_(buffer), input_(std::move(input)) {
    CHECK(session_);
    CHECK(buffer_);
  }

  class Lock final : public HasStatus, Sedentary {
   public:
    explicit Lock(DeviceFrameBuffer& parent)
        : registered_{NV_ENC_REGISTER_RESOURCE_VER,
                      NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR},
          mapped_{NV_ENC_MAP_INPUT_RESOURCE_VER},
          parent_{parent} {
      registered_.resourceToRegister = parent_.buffer_;
      registered_.bufferFormat = parent_.input_.format();
      registered_.pitch = parent_.input_.pitch_bytes();
      registered_.width = parent_.input_.width_bytes();
      registered_.height = parent_.input_.height();

      status_ = api().nvEncRegisterResource(parent_.session_, &registered_);
      if (has_error()) {
        LOG(ERROR) << "Failed to register input memory: " << error();
        return;
      }

      mapped_.registeredResource = registered_.registeredResource;
      status_ = api().nvEncMapInputResource(parent_.session_, &mapped_);
      if (has_error()) {
        LOG(ERROR) << "Failed to map registered memory: " << error();
      }
    }

    ~Lock() {
      status_ = api().nvEncUnmapInputResource(parent_.session_,
                                              mapped_.mappedResource);
      if (has_error()) {
        LOG(ERROR) << "Failed to unregister input memory: " << error();
      }

      status_ = api().nvEncUnregisterResource(parent_.session_,
                                              mapped_.registeredResource);
      if (has_error()) {
        LOG(ERROR) << "Failed to unregister input memory: " << error();
      }
    }

    size_t width() const { return registered_.width; }
    size_t height() const { return registered_.height; }
    size_t pitch() const { return registered_.pitch; }
    void* data() const { return mapped_.mappedResource; }
    NV_ENC_BUFFER_FORMAT format() const { return mapped_.mappedBufferFmt; }

   private:
    NV_ENC_REGISTER_RESOURCE registered_;
    NV_ENC_MAP_INPUT_RESOURCE mapped_;
    DeviceFrameBuffer& parent_;
  };

 private:
  void* session_;
  void* buffer_;
  EncoderInputFormat input_;
} class HostFrameBuffer final : public HasStatus, Sedentary {
 public:
  explicit HostFrameBuffer(const Session& session,
                           const EncoderInputFormat& input)
      : session_(session.handle()) {
    CHECK(api().nvEncCreateInputBuffer);
    CHECK(api().nvEncDestroyInputBuffer);
    CHECK(api().nvEncLockInputBuffer);
    CHECK(api().nvEncUnlockInputBuffer);

    NV_ENC_CREATE_INPUT_BUFFER create = {NV_ENC_CREATE_INPUT_BUFFER_VER};
    create.bufferFmt = input.format;
    create.width = input.width;
    create.height = input.height;

    status_ = api().nvEncCreateInputBuffer(session_, &create);
    if (has_error()) {
      LOG(ERROR) << "Failed to create input buffer: " << error();
    }

    buffer_ = create.inputBuffer;
  }

  ~HostFrameBuffer() {
    status_ = api().nvEncDestroyInputBuffer(session_, buffer_);
    if (has_error()) {
      LOG(ERROR) << "Failed to destroy input buffer: " << error();
    }
  }

  class Lock final : public HasStatus, Sedentary {
   public:
    explicit Lock(HostFrameBuffer& parent)
        : lock_{NV_ENC_LOCK_INPUT_BUFFER_VER}, parent_{parent} {
      lock_.inputBuffer = parent_.buffer_;

      status_ = api().nvEncLockInputBuffer(parent_.session_, &lock_);
      if (has_error()) {
        LOG(ERROR) << "Failed to lock input buffer: " << error();
      }
    }

    ~Lock() {
      status_ =
          api().nvEncUnlockInputBuffer(parent_.session_, lock_.inputBuffer);
      if (has_error()) {
        LOG(ERROR) << "Failed to unlock input buffer: " << error();
      }
    }

    uint8_t* data() const {
      return reinterpret_cast<uint8_t*>(lock_.bufferDataPtr);
    }
    uint32_t pitch() const { return lock_.pitch; }

   private:
    NV_ENC_LOCK_INPUT_BUFFER lock_;
    HostFrameBuffer& parent_;
  };

 private:
  void* session_ = nullptr;
  void* buffer_ = nullptr;
};

class EncodeBuffer final : public HasStatus, Sedentary {
 public:
  explicit EncodeBuffer(const Session& session) : session_(session.handle()) {
    CHECK(api().nvEncCreateBitstreamBuffer);
    CHECK(api().nvEncDestroyBitstreamBuffer);
    CHECK(api().nvEncLockBitstream);
    CHECK(api().nvEncUnlockBitstream);

    NV_ENC_CREATE_BITSTREAM_BUFFER output = {
        NV_ENC_CREATE_BITSTREAM_BUFFER_VER};

    status_ = api().nvEncCreateBitstreamBuffer(session_, &output);
    if (has_error()) {
      LOG(ERROR) << "Failed to create output buffer: " << error();
    }

    buffer_ = output.bitstreamBuffer;
  }

  ~EncodeBuffer() {
    status_ = api().nvEncDestroyBitstreamBuffer(session_, buffer_);
    if (has_error()) {
      LOG(ERROR) << "Failed to destroy output buffer: " << error();
    }
  }

  class DeviceWriteLock final : public HasStatus, Sedentary {
   public:
    explicit DeviceWriteLock(EncodeBuffer& parent) : parent_{parent} {}

    uint8_t* data() { return reinterpret_cast<uint8_t*>(parent_.buffer_); }

   private:
    EncodeBuffer& parent_;
  };

  class HostReadLock final : public HasStatus, Sedentary {
   public:
    explicit HostReadLock(EncodeBuffer& parent)
        : lock_{NV_ENC_LOCK_BITSTREAM_VER}, parent_{parent} {
      lock_.outputBitstream = parent_.buffer_;

      status_ = api().nvEncLockBitstream(parent_.session_, &lock_);
      if (has_error()) {
        LOG(ERROR) << "Failed to lock output bitstream: " << error();
      }
    }

    ~HostReadLock() {
      auto status_ =
          api().nvEncUnlockBitstream(parent_.session_, lock_.outputBitstream);
      if (has_error()) {
        LOG(ERROR) << "Failed to unlock output bitstream: " << error();
      }
    }

    const uint8_t* data() const {
      return reinterpret_cast<const uint8_t*>(lock_.bitstreamBufferPtr);
    }

    uint32_t size() const { return lock_.bitstreamSizeInBytes; }

   private:
    NV_ENC_LOCK_BITSTREAM lock_;
    EncodeBuffer& parent_;
  };

 private:
  void* session_ = nullptr;
  void* buffer_ = nullptr;
};

class Encoder final : public HasStatus, Sedentary {
 public:
  explicit Encoder(
      const Session& session, EncoderInputFormat input,
      EncoderOutputFormat encode,
      std::function<void(NV_ENC_CONFIG&, NVENCSTATUS)> customize = {})
      : session_(session.handle()),
        input_(std::move(input)),
        output_(std::move(encode)) {
    CHECK(api().nvEncInitializeEncoder);
    CHECK(api().nvEncGetEncodePresetConfig);
    CHECK(api().nvEncEncodePicture);
    CHECK(api().nvEncDestroyEncoder);

    NV_ENC_CONFIG config = presets(output_).presetCfg;
    if (customize) {
      customize(config, status_);
    }

    NV_ENC_INITIALIZE_PARAMS init = {NV_ENC_INITIALIZE_PARAMS_VER};
    init.encodeConfig = &config;
    init.encodeGUID = output_.codec;
    init.presetGUID = output_.preset;
    init.maxEncodeWidth = init.encodeWidth = input_.width;
    init.maxEncodeHeight = init.encodeHeight = input_.height;
    init.frameRateNum = input_.frames_per_second;
    init.frameRateDen = 1;
    init.enablePTD = 1;

    status_ = api().nvEncInitializeEncoder(session_, &init);
    if (has_error()) {
      LOG(ERROR) << "Failed to initialize encoder: " << error();
    }
  }

  ~Encoder() {
    status_ = api().nvEncDestroyEncoder(session_);
    if (has_error()) {
      LOG(ERROR) << "Failed to destroy input buffer: " << error();
    }
  }

  void encode(void* input_buffer, uint32_t input_pitch, void* output_buffer) {
    NV_ENC_PIC_PARAMS encoding = {NV_ENC_PIC_PARAMS_VER};

    encoding.bufferFmt = input_.format();
    encoding.inputWidth = input_.width_bytes();
    encoding.inputHeight = input_.height();
    encoding.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;  // progressive

    encoding.inputBuffer = input_buffer;
    encoding.inputPitch = input_pitch;
    encoding.outputBitstream = output_buffer;

    status_ = api().nvEncEncodePicture(session_, &encoding);
    switch (status_) {
      case NV_ENC_SUCCESS:              // ok!
      case NV_ENC_ERR_NEED_MORE_INPUT:  // B-frame!
        break;
      default:
        LOG(ERROR) << "Failed to encode picture: " << error();
    }
  }

  const EncoderInputFormat& takes() const { return input_; }
  const EncoderOutputFormat& gives() const { return output_; }

 private:
  NV_ENC_PRESET_CONFIG presets(const EncoderOutputFormat& encode) {
    NV_ENC_PRESET_CONFIG result = {NV_ENC_PRESET_CONFIG_VER};
    result.presetCfg.version = NV_ENC_CONFIG_VER;

    status_ = api().nvEncGetEncodePresetConfig(session_, encode.codec,
                                               encode.preset, &result);
    if (has_error()) {
      LOG(ERROR) << "Failed to load presets: " << error();
    }

    return result;
  }

  void* session_ = nullptr;
  EncoderInputFormat input_;
  EncoderOutputFormat output_;
};

}  // namespace nvenc

namespace y4m {
// Basic parsing for YUV4MPEG2 uncompressed video format
// - format is tiny header with uncompressed YUV420p frames

template <typename Type>
struct Literal {
  Literal(const Literal&) = default;
  Literal& operator=(const Literal&) = default;

  Literal(Literal&&) = default;
  Literal& operator=(Literal&&) = default;

  Literal(Type&& literal) : value{std::forward<Type>(literal)} {}

  const Type value;
};

struct Geometry {
  size_t width = 0;
  size_t height = 0;
};

struct FrameRate {
  size_t frames = 0;
  size_t seconds = 1;
};

enum class Interlacing { progressive, top_first, bottom_first, mixed_mode };
enum class ColorSpace { yuv420, yuv422, yuv444 };

struct AspectRatio {
  size_t numerator = 0;
  size_t denominator = 0;
};

struct Comment {
  std::string text;
};

struct Frame {
  Frame(size_t size) { image.resize(size); }

  Frame(const Frame&) = default;
  Frame& operator=(const Frame&) = default;

  Frame(Frame&&) = default;
  Frame& operator=(Frame&&) = default;

  std::string image;
  std::string parameters;
};

std::istream& operator>>(std::istream& stream, Literal<std::string> literal) {
  if (stream) {
    std::string buffer(literal.value.size(), '\0');
    auto prev = stream.width(literal.value.size());
    stream >> buffer;
    stream.width(prev);
    if (buffer != literal.value) {
      stream.setstate(std::ios_base::failbit);
    }
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, Literal<char> literal) {
  if (stream) {
    char ch = '\0';
    stream >> ch;
    if (ch != literal.value) {
      stream.setstate(std::ios_base::failbit);
    }
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, Geometry& geometry) {
  if (stream) {
    stream >> Literal<char>{'W'} >> geometry.width >> Literal<char>{'H'} >>
        geometry.height;
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, FrameRate& framerate) {
  if (stream) {
    stream >> Literal<char>{'F'} >> framerate.frames >> Literal<char>{':'} >>
        framerate.seconds;
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, Interlacing& interlacing) {
  if (stream) {
    char ch;
    stream >> Literal<char>{'I'} >> ch;
    switch (ch) {
      case 'p':
        interlacing = Interlacing::progressive;
        break;
      case 't':
        interlacing = Interlacing::top_first;
        break;
      case 'b':
        interlacing = Interlacing::bottom_first;
        break;
      case 'm':
        interlacing = Interlacing::mixed_mode;
        break;
    }
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, AspectRatio& ratio) {
  if (stream) {
    stream >> Literal<char>{'A'} >> ratio.numerator >> Literal<char>{':'} >>
        ratio.denominator;
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, ColorSpace& colorspace) {
  if (stream) {
    std::string buffer;
    stream >> Literal<char>{'C'} >> buffer;
    if ((buffer == "420jpeg") || (buffer == "420paldv") || (buffer == "420")) {
      colorspace = ColorSpace::yuv420;
    } else if (buffer == "422") {
      colorspace = ColorSpace::yuv422;
    } else if (buffer == "444") {
      colorspace = ColorSpace::yuv444;
    } else {
      stream.setstate(std::ios_base::failbit);
    }
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, Comment& comment) {
  if (stream) {
    stream >> Literal<char>{'X'} >> comment.text;
  }
  return stream;
}

std::istream& operator>>(std::istream& stream, Frame& frame) {
  if (stream) {
    stream >> Literal<std::string>{"FRAME"};
    if (stream.peek() != '\n') {
      stream >> frame.parameters;
    } else {
      stream.ignore();
    }
    stream.read(&frame.image[0], frame.image.size());
  }
  return stream;
}

}  // namespace y4m

namespace {
// Taken from NvEncAPIv8.0; new names for previous values
static constexpr int NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ = 0x8;
static constexpr int NV_ENC_PARAMS_RC_CBR_HQ = 0x10;
static constexpr int NV_ENC_PARAMS_RC_VBR_HQ = 0x20;

std::string to_string(NV_ENC_BUFFER_FORMAT format) {
  switch (format) {
    case NV_ENC_BUFFER_FORMAT_NV12:
      return "NV_ENC_BUFFER_FORMAT_NV12";
    case NV_ENC_BUFFER_FORMAT_YV12:
      return "NV_ENC_BUFFER_FORMAT_YV12";
    case NV_ENC_BUFFER_FORMAT_IYUV:
      return "NV_ENC_BUFFER_FORMAT_IYUV";
    case NV_ENC_BUFFER_FORMAT_YUV444:
      return "NV_ENC_BUFFER_FORMAT_YUV444";
    case NV_ENC_BUFFER_FORMAT_YUV420_10BIT:
      return "NV_ENC_BUFFER_FORMAT_YUV420_10BIT";
    case NV_ENC_BUFFER_FORMAT_YUV444_10BIT:
      return "NV_ENC_BUFFER_FORMAT_YUV444_10BIT";
    case NV_ENC_BUFFER_FORMAT_ARGB:
      return "NV_ENC_BUFFER_FORMAT_ARGB";
    case NV_ENC_BUFFER_FORMAT_ARGB10:
      return "NV_ENC_BUFFER_FORMAT_ARGB10";
    case NV_ENC_BUFFER_FORMAT_AYUV:
      return "NV_ENC_BUFFER_FORMAT_AYUV";
    case NV_ENC_BUFFER_FORMAT_ABGR:
      return "NV_ENC_BUFFER_FORMAT_ABGR";
    case NV_ENC_BUFFER_FORMAT_ABGR10:
      return "NV_ENC_BUFFER_FORMAT_ABGR10";
    default:
      return "NV_ENC_BUFFER_FORMAT_UNDEFINED";
  }
}

size_t size(std::istream& stream) {
  auto curr = stream.tellg();
  stream.seekg(0, std::ios::beg);
  auto begin = stream.tellg();
  stream.seekg(0, std::ios::end);
  auto end = stream.tellg();
  stream.seekg(curr);
  return end - begin;
}

size_t size_remaining(std::istream& stream) {
  auto curr = stream.tellg();
  stream.seekg(0, std::ios::end);
  auto end = stream.tellg();
  stream.seekg(curr);
  return end - curr;
}

char peek_next_char(std::istream& stream) {
  char ch = '\0';
  if (stream) {
    auto pos = stream.tellg();
    stream >> ch;
    stream.seekg(pos);
  }
  return ch;
}

struct NetworkUnit {
  NetworkUnit(uint8_t* data) : data{data} {}

  NetworkUnit(const NetworkUnit&) = default;
  NetworkUnit& operator=(const NetworkUnit&) = default;

  NetworkUnit(NetworkUnit&&) = default;
  NetworkUnit& operator=(NetworkUnit&&) = default;

  uint8_t* data = nullptr;
  size_t size = 0;
};

std::vector<NetworkUnit> nal_parse_units(void* buffer, size_t size) {
  std::vector<NetworkUnit> units;

  auto ptr = reinterpret_cast<uint8_t*>(buffer);
  auto end = ptr + size;
  while (ptr != end) {
    ptr = std::find(ptr, end, 0);

    auto d = std::distance(ptr, end);
    auto h1 = d >= 3 && ptr[1] == 0 && ptr[2] == 1;
    auto h2 = d >= 4 && ptr[1] == 0 && ptr[2] == 0 && ptr[3] == 1;
    auto offset = h1 ? 3 : h2 ? 4 : ptr != end ? 1 : 0;

    switch (offset) {
      case 3:
      case 4:
        if (units.size()) {
          units.back().size = std::distance(units.back().data, ptr);
        }
        ptr += offset;
        units.push_back({ptr});
        break;
      default:
        ptr += offset;
    }
  }

  if (units.size()) {
    units.back().size = std::distance(units.back().data, end);
  }

  return units;
}

struct NvencDriverLibrary final : public dll::DynamicLibrary {
 public:
  NvencDriverLibrary()
      : dll::DynamicLibrary("libnvidia-encode.so.1"),
        EncodeAPICreateInstance(*this, "NvEncodeAPICreateInstance") {}

  dll::Function<decltype(NvEncodeAPICreateInstance)> EncodeAPICreateInstance;
};

auto segment(int64_t total_count, int64_t group_count) {
  auto&& result = std::div(total_count, group_count);
  return result.quot + static_cast<bool>(result.rem);
}

template <typename Type>
__device__ Type clamp(const Type value, const Type a, const Type b) {
  return max(a, min(value, b));
}

template <typename Type>
__device__ Type peek(const Type* data, const int W, const int H, const int x,
                     const int y) {
  return data[W * y + x];
}

__global__ void unpack_to_yuv444p_2x2(const uint16_t* in, const int W,
                                      const int H, uint16_t* luma,
                                      uint16_t* chroma_r, uint16_t* chroma_b) {
  const int half_W = W / 2;
  const int half_x = blockDim.x * blockIdx.x + threadIdx.x;
  const int half_y = blockDim.y * blockIdx.y + threadIdx.y;
  const int x = 2 * half_x;
  const int y = 2 * half_y;

  if (x > (W - 2) || y > (H - 2)) {
    return;
  }

  // ...
}
void convert_to_yuv444p_cuda(const uint16_t* int, const size_t W,
                             const size_t H, uint16_t* y, uint16_t* u,
                             uint16_t* v) {
  CHECK(W < std::numeric_limits<decltype(W)>::max());
  CHECK(H < std::numeric_limits<decltype(H)>::max());
  dim3 threads(8, 8);  // multiple of 32
  dim3 blocks(segment(W / 2, threads.x), segment(H / 2, threads.y));
  unpack_to_yuv444p_2x2<<<blocks, threads>>>(in, static_cast<int>(W),
                                             static_cast<int>(H), y, u, v);

  if (cudaDeviceSynchronize() != cudaSuccess) {
    LOG(ERROR) << "Failed to sync cuda";
  }
}  // namespace

int main(int argc, char** argv) {
  LOG(INFO) << "Hello World.";

  if (argc != 3) {
    LOG(ERROR) << argv[0] << " <input file> <output file>";
    return -1;
  }

  std::ifstream video_y4m{argv[1]};
  if (!video_y4m) {
    LOG(ERROR) << "Unable to open input file";
    return -1;
  }

  std::ofstream video_hevc{argv[2]};
  if (!video_hevc) {
    LOG(ERROR) << "Unable to open output file";
    return -1;
  }

  y4m::Geometry geometry;
  y4m::FrameRate framerate;
  y4m::Interlacing interlacing;
  y4m::AspectRatio ratio;

  video_y4m >> Literal<std::string>{"YUV4MPEG2"} >> geometry >> framerate >>
      interlacing >> ratio;

  if (peek_next_char(video_y4m) == 'C') {
    ColorSpace colorspace;
    video_y4m >> colorspace;
  }

  if (peek_next_char(video_y4m) == 'X') {
    Comment comment;
    video_y4m >> comment;
  }

  if (!video_y4m) {
    LOG(ERROR) << "Error parsing video";
    return -1;
  }

  LOG(INFO) << geometry.width << "x" << geometry.height << " pixels\n";
  LOG(INFO) << (framerate.frames / framerate.seconds) << " fps\n";
  LOG(INFO) << ratio.numerator << ":" << ratio.denominator << "\n";

  // NVENC is driver-only
  NvencDriverLibrary nvlib;
  if (!nvlib) {
    LOG(ERROR) << "nvenc library load failed: " << nvlib.error();
    return -1;
  }

  auto cuerr = cudaDeviceSynchronize();
  if (cuerr != cudaSuccess) {
    LOG(ERROR) << "Error syncing device: " << cuerr;
    return -1;
  }

  CUcontext cucontext;
  auto curesult = cuCtxGetCurrent(&cucontext);
  if (curesult != CUDA_SUCCESS) {
    LOG(ERROR) << "Error getting current context: " << curesult;
    return -1;
  }

  auto nverr = nvlib.EncodeAPICreateInstance(&nvenc::api());
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Error loading encode functions: " << nvenc::statusstr(nverr);
    return -1;
  }

  bool has_hevc = false;
  uint32_t count = 0;
  nverr = nvenc::api().nvEncGetEncodeGUIDCount(encoder, &count);
  std::unique_ptr<GUID[]> guids{new GUID[count]};
  nverr = nvenc::api().nvEncGetEncodeGUIDs(encoder, guids.get(), count, &count);
  for (uint32_t i = 0; i < count && !has_hevc; ++i) {
    if (std::memcmp(&guids[i], &NV_ENC_CODEC_HEVC_GUID, sizeof(GUID)) == 0) {
      has_hevc = true;
    }
  }

  if (!has_hevc) {
    LOG(ERROR) << "Video card does not support HEVC";
    return -1;
  }

  for (NV_ENC_CAPS what : {
           NV_ENC_CAPS_NUM_MAX_BFRAMES,
           NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES,
           NV_ENC_CAPS_SUPPORT_FIELD_ENCODING,
           NV_ENC_CAPS_SUPPORT_MONOCHROME,
           NV_ENC_CAPS_SUPPORT_FMO,
           NV_ENC_CAPS_SUPPORT_QPELMV,
           NV_ENC_CAPS_SUPPORT_BDIRECT_MODE,
           NV_ENC_CAPS_SUPPORT_CABAC,
           NV_ENC_CAPS_SUPPORT_ADAPTIVE_TRANSFORM,
           NV_ENC_CAPS_SUPPORT_RESERVED,
           NV_ENC_CAPS_NUM_MAX_TEMPORAL_LAYERS,
           NV_ENC_CAPS_SUPPORT_HIERARCHICAL_PFRAMES,
           NV_ENC_CAPS_SUPPORT_HIERARCHICAL_BFRAMES,
           NV_ENC_CAPS_LEVEL_MAX,
           NV_ENC_CAPS_LEVEL_MIN,
           NV_ENC_CAPS_SEPARATE_COLOUR_PLANE,
           NV_ENC_CAPS_WIDTH_MAX,
           NV_ENC_CAPS_HEIGHT_MAX,
           NV_ENC_CAPS_SUPPORT_TEMPORAL_SVC,
           NV_ENC_CAPS_SUPPORT_DYN_RES_CHANGE,
           NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE,
           NV_ENC_CAPS_SUPPORT_DYN_FORCE_CONSTQP,
           NV_ENC_CAPS_SUPPORT_DYN_RCMODE_CHANGE,
           NV_ENC_CAPS_SUPPORT_SUBFRAME_READBACK,
           NV_ENC_CAPS_SUPPORT_CONSTRAINED_ENCODING,
           NV_ENC_CAPS_SUPPORT_INTRA_REFRESH,
           NV_ENC_CAPS_SUPPORT_CUSTOM_VBV_BUF_SIZE,
           NV_ENC_CAPS_SUPPORT_DYNAMIC_SLICE_MODE,
           NV_ENC_CAPS_SUPPORT_REF_PIC_INVALIDATION,
           NV_ENC_CAPS_PREPROC_SUPPORT,
           NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT,
           NV_ENC_CAPS_MB_NUM_MAX,
           NV_ENC_CAPS_MB_PER_SEC_MAX,
           NV_ENC_CAPS_SUPPORT_YUV444_ENCODE,
           NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE,
           NV_ENC_CAPS_SUPPORT_SAO,
           NV_ENC_CAPS_SUPPORT_MEONLY_MODE,
           NV_ENC_CAPS_SUPPORT_LOOKAHEAD,
           NV_ENC_CAPS_SUPPORT_10BIT_ENCODE,
       }) {
    NV_ENC_CAPS_PARAM caps;
    std::memset(&caps, 0, sizeof(caps));
    caps.version = NV_ENC_CAPS_PARAM_VER;
    caps.capsToQuery = what;

    int value = 0;
    nverr = nvenc::api().nvEncGetEncodeCaps(encoder, NV_ENC_CODEC_HEVC_GUID,
                                            &caps, &value);

    if (nverr == NV_ENC_SUCCESS) {
      LOG(INFO) << "capability " << what << " is " << value;
    } else {
      LOG(ERROR) << "Failed to query capability " << what;
      return -1;
    }
  }

  nverr = nvenc::api().nvEncGetInputFormatCount(encoder, NV_ENC_CODEC_HEVC_GUID,
                                                &count);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to get input format count";
    return -1;
  }

  std::unique_ptr<NV_ENC_BUFFER_FORMAT[]> formats{
      new NV_ENC_BUFFER_FORMAT[count]};
  nverr = nvenc::api().nvEncGetInputFormats(encoder, NV_ENC_CODEC_HEVC_GUID,
                                            formats.get(), count, &count);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to get input formats";
    return -1;
  }

  for (int i = 0; i < count; ++i) {
    LOG(ERROR) << "Supported input formats: " << to_string(formats[i]);
  }

  NV_ENC_CONFIG config;
  memset(&config, 0, sizeof(config));
  memcpy(&config, &preset.presetCfg, sizeof(config));

  const uint32_t Mbps = 1024 * 1024;
  const uint32_t yuv420 = 1;

  config.version = NV_ENC_CONFIG_VER;
  config.profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
  config.gopLength = NVENC_INFINITE_GOPLENGTH;  // Use Infra Refresh
  config.frameIntervalP = 1;                    // Use I and P frames only

  config.rcParams.version = NV_ENC_RC_PARAMS_VER;
  config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
  config.rcParams.constQP.qpInterP = 36;
  config.rcParams.constQP.qpIntra = 46;
  config.rcParams.enableAQ = 1;

  config.encodeCodecConfig.hevcConfig.tier = NV_ENC_TIER_HEVC_HIGH;
  config.encodeCodecConfig.hevcConfig.level = NV_ENC_LEVEL_AUTOSELECT;
  config.encodeCodecConfig.hevcConfig.chromaFormatIDC = yuv420;  // input

  // Divide frame into independently decodable units
  config.encodeCodecConfig.hevcConfig.sliceMode = 1;
  config.encodeCodecConfig.hevcConfig.sliceModeData = 1400;  // MTU

  // Rolling intra slices for packetization to replace monolithic keyframes
  config.encodeCodecConfig.hevcConfig.enableIntraRefresh = 1;  // Periodic IR
  config.encodeCodecConfig.hevcConfig.intraRefreshPeriod = 6;
  config.encodeCodecConfig.hevcConfig.intraRefreshCnt = 4;

  // Output VPS,SPS and PPS for every IDR frame
  config.encodeCodecConfig.hevcConfig.repeatSPSPPS = 1;

  // Long term reference pictures for inter prediction (error resilience)
  // Deprecated? Will need to manually specifiy which are LTR?
  config.encodeCodecConfig.hevcConfig.enableLTR = 1;
  config.encodeCodecConfig.hevcConfig.ltrTrustMode = 1;
  config.encodeCodecConfig.hevcConfig.ltrNumFrames = 10;

  NV_ENC_INITIALIZE_PARAMS init;
  memset(&init, 0, sizeof(init));
  init.version = NV_ENC_INITIALIZE_PARAMS_VER;
  init.encodeConfig = &config;
  init.encodeGUID = NV_ENC_CODEC_HEVC_GUID;
  init.presetGUID = NV_ENC_PRESET_HQ_GUID;  // Defaults
  init.maxEncodeWidth = init.encodeWidth = init.darWidth = geometry.width;
  init.maxEncodeHeight = init.encodeHeight = init.darHeight = geometry.height;
  init.frameRateNum = framerate.frames;
  init.frameRateDen = framerate.seconds;
  init.enablePTD = 1;  // send frames in display order

  nverr = nvenc::api().nvEncInitializeEncoder(encoder, &init);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to initialize encoder: " << nvcodec_strerror(nverr);
  }

  // Sadly CPU copy on in/out seems unavoidable
  // Input buffer must be as below or cuMemAlloc
  NV_ENC_CREATE_INPUT_BUFFER input;
  memset(&input, 0, sizeof(input));

  input.version = NV_ENC_CREATE_INPUT_BUFFER_VER;
  input.bufferFmt = NV_ENC_BUFFER_FORMAT_IYUV;
  input.width = geometry.width;
  input.height = geometry.height;
  input.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

  nverr = nvenc::api().nvEncCreateInputBuffer(encoder, &input);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Error creating input buffer";
  }

  NV_ENC_CREATE_BITSTREAM_BUFFER output;
  memset(&output, 0, sizeof(output));

  output.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
  output.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
  output.size = init.encodeHeight * init.encodeWidth * 4;  // oversize

  nverr = nvenc::api().nvEncCreateBitstreamBuffer(encoder, &output);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Error creating output output";
  }

  auto frames_total = 0;
  auto packets_total = 0;
  auto packets_lost = 0;
  auto packet_loss = 0.02;

  std::random_device randev;
  std::seed_seq randseed{randev(), randev(), randev(), randev()};
  std::mt19937 randgen{randseed};
  std::geometric_distribution<> geometric{1.0 - packet_loss};

  // http://www.fourcc.org/pixel-format/yuv-yv12/
  y4m::Frame frame(geometry.width * geometry.height * 1.5);

  for (frames_total = 0; video_y4m; frames_total++) {
    video_y4m >> frame;

    NV_ENC_LOCK_INPUT_BUFFER input_lock;
    memset(&input_lock, 0, sizeof(input_lock));

    input_lock.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
    input_lock.inputBuffer = input.inputBuffer;

    nverr = nvenc::api().nvEncLockInputBuffer(encoder, &input_lock);
    if (nverr != NV_ENC_SUCCESS) {
      LOG(ERROR) << "Failed to lock input buffer";
    }

    // Hardware will ask for pitch aligned memory
    const size_t in_stride = geometry.width;
    const size_t in_luma_size = in_stride * geometry.height;
    const size_t in_chroma_size = in_luma_size / 4;
    const size_t in_lines = geometry.height;

    const size_t hw_stride = input_lock.pitch;
    const size_t hw_luma_size = hw_stride * geometry.height;
    const size_t hw_chroma_size = hw_luma_size / 4;

    auto in_luma_buf = frame.image.data();
    auto in_chromab_buf = in_luma_buf + in_luma_size;
    auto in_chromar_buf = in_chromab_buf + in_chroma_size;

    auto hw_luma_buf = reinterpret_cast<char*>(input_lock.bufferDataPtr);
    auto hw_chromab_buf = hw_luma_buf + hw_luma_size;
    auto hw_chromar_buf = hw_chromab_buf + hw_chroma_size;

    // Prefer memory locality
    for (int i = 0; i < in_lines; i++) {
      std::copy_n(in_luma_buf + in_stride * i, in_stride,
                  hw_luma_buf + hw_stride * i);
    }

    for (int i = 0; i < in_lines / 2; i++) {
      std::copy_n(in_chromab_buf + in_stride * i / 2, in_stride / 2,
                  hw_chromab_buf + hw_stride * i / 2);
    }

    for (int i = 0; i < in_lines / 2; i++) {
      std::copy_n(in_chromar_buf + in_stride * i / 2, in_stride / 2,
                  hw_chromar_buf + hw_stride * i / 2);
    }

    nverr =
        nvenc::api().nvEncUnlockInputBuffer(encoder, input_lock.inputBuffer);
    if (nverr != NV_ENC_SUCCESS) {
      LOG(ERROR) << "Failed to unlock input buffer";
    }

    NV_ENC_PIC_PARAMS encoding;
    memset(&encoding, 0, sizeof(encoding));

    encoding.version = NV_ENC_PIC_PARAMS_VER;
    encoding.inputBuffer = input.inputBuffer;
    encoding.bufferFmt = input.bufferFmt;
    encoding.inputWidth = input.width;
    encoding.inputHeight = input.height;
    encoding.inputPitch = input_lock.pitch;
    encoding.outputBitstream = output.bitstreamBuffer;
    encoding.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;  // progressive

    nverr = nvenc::api().nvEncEncodePicture(encoder, &encoding);
    if (nverr != NV_ENC_SUCCESS) {
      // Will not return NV_ENC_ERR_NEED_MORE_INPUT; no B-frames
      LOG(ERROR) << "Failed to encode encoding";
    }

    NV_ENC_LOCK_BITSTREAM output_lock;
    memset(&output_lock, 0, sizeof(output_lock));

    output_lock.version = NV_ENC_LOCK_BITSTREAM_VER;
    output_lock.outputBitstream = output.bitstreamBuffer;

    nverr = nvenc::api().nvEncLockBitstream(encoder, &output_lock);
    if (nverr != NV_ENC_SUCCESS) {
      LOG(ERROR) << "Failed to lock output bitstream";
    }

    auto units = nal_parse_units(output_lock.bitstreamBufferPtr,
                                 output_lock.bitstreamSizeInBytes);

    packets_total += units.size();
    for (size_t i = 0; i < units.size(); ++i) {
      auto burst = geometric(randgen);
      packets_lost += burst;
      for (; burst && i < units.size(); --burst, ++i) {
        std::fill_n(units[i].data, units[i].size, 0);
      }
    }

    video_hevc.write(
        reinterpret_cast<const char*>(output_lock.bitstreamBufferPtr),
        output_lock.bitstreamSizeInBytes);

    nverr =
        nvenc::api().nvEncUnlockBitstream(encoder, output_lock.outputBitstream);
    if (nverr != NV_ENC_SUCCESS) {
      LOG(ERROR) << "Failed to lock output bitstream";
    }
  }

  LOG(INFO) << "Encoded " << frames_total << " frames";
  LOG(INFO) << "With " << 100.f * packets_lost / packets_total
            << "% packet loss";

  nverr = nvenc::api().nvEncDestroyInputBuffer(encoder, input.inputBuffer);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to destroy input buffer";
  }

  nverr =
      nvenc::api().nvEncDestroyBitstreamBuffer(encoder, output.bitstreamBuffer);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to destroy input buffer";
  }

  nverr = nvenc::api().nvEncDestroyEncoder(encoder);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to destroy input buffer";
  }

  LOG(INFO) << "Thanks.";
}
