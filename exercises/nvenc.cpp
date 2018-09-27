#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>
#include <limits>
#include <memory>
#include <random>

#include <dlfcn.h>
#include <cuda.h>
#include <cuda_runtime_api.h>

#include "glog/logging.h"

#include "compat/nvenc/nvEncodeAPI.h"  // from ffmpeg
#include "compat/cuda/dynlink_cuda.h"

namespace {

// Taken from NvEncAPIv8.0; new names for previous values
static constexpr int NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ = 0x8;
static constexpr int NV_ENC_PARAMS_RC_CBR_HQ = 0x10;
static constexpr int NV_ENC_PARAMS_RC_VBR_HQ = 0x20;

const char* NvCodecEncErrors[] = {
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

const char* nvcodec_strerror(NVENCSTATUS err) { return NvCodecEncErrors[err]; }

// Basic parsing for YUV4MPEG2 uncompressed video format
// - format is tiny header with uncompressed YUV420p frames

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

struct NetworkUnit {
  NetworkUnit(uint8_t* data) : data{data} {}

  NetworkUnit(const NetworkUnit&) = default;
  NetworkUnit& operator=(const NetworkUnit&) = default;

  NetworkUnit(NetworkUnit&&) = default;
  NetworkUnit& operator=(NetworkUnit&&) = default;

  uint8_t* data = nullptr;
  size_t size = 0;
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

  Geometry geometry;
  FrameRate framerate;
  Interlacing interlacing;
  AspectRatio ratio;

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

  auto nvenc_api = dlopen("libnvidia-encode.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (!nvenc_api) {
    LOG(ERROR) << "dlopen failed";
    return -1;
  }

  auto NvEncodeAPICreateInstance = reinterpret_cast<NVENCSTATUS (*)(void*)>(
      dlsym(nvenc_api, "NvEncodeAPICreateInstance"));
  if (!NvEncodeAPICreateInstance) {
    LOG(ERROR) << "dlsym failed";
    return -1;
  }

  NV_ENCODE_API_FUNCTION_LIST nvenc;
  memset(&nvenc, 0, sizeof(nvenc));
  nvenc.version = NV_ENCODE_API_FUNCTION_LIST_VER;

  auto nverr = NvEncodeAPICreateInstance(&nvenc);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Error loading encode functions: " << nvcodec_strerror(nverr);
    return -1;
  }

  // Driver API is needed for cu* functions
  auto cuda_driver_api = dlopen("libcuda.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (!cuda_driver_api) {
    LOG(ERROR) << "dlopen failed";
    return -1;
  }

  auto cuCtxGetCurrent = reinterpret_cast<CUresult (*)(CUcontext*)>(
      dlsym(cuda_driver_api, "cuCtxGetCurrent"));
  if (!cuCtxGetCurrent) {
    LOG(ERROR) << "dlsym failed";
    return -1;
  }

  auto cuerr = cudaDeviceSynchronize();
  if (cuerr != cudaSuccess) {
    LOG(ERROR) << "Error syncing device: " << cuerr;
    return -1;
  }

  CUcontext cuda_ctx;
  auto curesult = cuCtxGetCurrent(&cuda_ctx);
  if (curesult != CUDA_SUCCESS) {
    LOG(ERROR) << "Error getting current context: " << curesult;
    return -1;
  }

  NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params;
  memset(&params, 0, sizeof(params));
  params.apiVersion = NVENCAPI_VERSION;
  params.device = cuda_ctx;
  params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
  params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;

  using NvencEncoder = void*;
  NvencEncoder encoder;
  nverr = nvenc.nvEncOpenEncodeSessionEx(&params, &encoder);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to open encode session";
  }

  bool has_hevc = false;
  uint32_t count = 0;
  nverr = nvenc.nvEncGetEncodeGUIDCount(encoder, &count);
  std::unique_ptr<GUID[]> guids{new GUID[count]};
  nverr = nvenc.nvEncGetEncodeGUIDs(encoder, guids.get(), count, &count);
  for (uint32_t i = 0; i < count && !has_hevc; ++i) {
    if (memcmp(&guids[i], &NV_ENC_CODEC_HEVC_GUID, sizeof(GUID)) == 0) {
      has_hevc = true;
    }
  }

  if (!has_hevc) {
    LOG(ERROR) << "Video card does not support HEVC";
    return -1;
  }

  // GeForce GTX 1070: CUDA 6.1
  for (NV_ENC_CAPS what : {
           NV_ENC_CAPS_NUM_MAX_BFRAMES,               // 0
           NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES,   // 63
           NV_ENC_CAPS_SUPPORT_FIELD_ENCODING,        // 0
           NV_ENC_CAPS_SUPPORT_MONOCHROME,            // 0
           NV_ENC_CAPS_SUPPORT_FMO,                   // 0
           NV_ENC_CAPS_SUPPORT_QPELMV,                // 1
           NV_ENC_CAPS_SUPPORT_BDIRECT_MODE,          // 0
           NV_ENC_CAPS_SUPPORT_CABAC,                 // 1
           NV_ENC_CAPS_SUPPORT_ADAPTIVE_TRANSFORM,    // 0
           NV_ENC_CAPS_SUPPORT_RESERVED,              // 0
           NV_ENC_CAPS_NUM_MAX_TEMPORAL_LAYERS,       // 0
           NV_ENC_CAPS_SUPPORT_HIERARCHICAL_PFRAMES,  // 0
           NV_ENC_CAPS_SUPPORT_HIERARCHICAL_BFRAMES,  // 0
           NV_ENC_CAPS_LEVEL_MAX,                     // 62
           NV_ENC_CAPS_LEVEL_MIN,                     // 1
           NV_ENC_CAPS_SEPARATE_COLOUR_PLANE,         // 0
           NV_ENC_CAPS_WIDTH_MAX,                     // 8k
           NV_ENC_CAPS_HEIGHT_MAX,                    // 8k
           NV_ENC_CAPS_SUPPORT_TEMPORAL_SVC,          // 0
           NV_ENC_CAPS_SUPPORT_DYN_RES_CHANGE,        // 1
           NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE,    // 1
           NV_ENC_CAPS_SUPPORT_DYN_FORCE_CONSTQP,     // 1
           NV_ENC_CAPS_SUPPORT_DYN_RCMODE_CHANGE,     // 0
           NV_ENC_CAPS_SUPPORT_SUBFRAME_READBACK,     // 1
           NV_ENC_CAPS_SUPPORT_CONSTRAINED_ENCODING,  // 0
           NV_ENC_CAPS_SUPPORT_INTRA_REFRESH,         // 1
           NV_ENC_CAPS_SUPPORT_CUSTOM_VBV_BUF_SIZE,   // 1
           NV_ENC_CAPS_SUPPORT_DYNAMIC_SLICE_MODE,    // 1
           NV_ENC_CAPS_SUPPORT_REF_PIC_INVALIDATION,  // 1
           NV_ENC_CAPS_PREPROC_SUPPORT,               // 0
           NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT,          // 0
           NV_ENC_CAPS_MB_NUM_MAX,                    // 256k
           NV_ENC_CAPS_MB_PER_SEC_MAX,                // 960k
           NV_ENC_CAPS_SUPPORT_YUV444_ENCODE,         // 1
           NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE,       // 1
           NV_ENC_CAPS_SUPPORT_SAO,                   // 1
           NV_ENC_CAPS_SUPPORT_MEONLY_MODE,           // 1
           NV_ENC_CAPS_SUPPORT_LOOKAHEAD,             // 1
           NV_ENC_CAPS_SUPPORT_10BIT_ENCODE,          // 1
       }) {
    NV_ENC_CAPS_PARAM caps;
    memset(&caps, 0, sizeof(caps));
    caps.version = NV_ENC_CAPS_PARAM_VER;
    caps.capsToQuery = what;

    int value = 0;
    nverr = nvenc.nvEncGetEncodeCaps(encoder, NV_ENC_CODEC_HEVC_GUID, &caps,
                                     &value);

    if (nverr == NV_ENC_SUCCESS) {
      // LOG(INFO) << "capability " << what << " is " << value;
    } else {
      LOG(ERROR) << "Failed to query capability " << what;
      return -1;
    }
  }

  NV_ENC_PRESET_CONFIG preset = {0};
  preset.version = NV_ENC_PRESET_CONFIG_VER;
  preset.presetCfg.version = NV_ENC_CONFIG_VER;
  nverr = nvenc.nvEncGetEncodePresetConfig(encoder, NV_ENC_CODEC_HEVC_GUID,
                                           NV_ENC_PRESET_HQ_GUID, &preset);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to get encode preset config";
    return -1;
  }

  nverr =
      nvenc.nvEncGetInputFormatCount(encoder, NV_ENC_CODEC_HEVC_GUID, &count);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to get input format count";
    return -1;
  }

  std::unique_ptr<NV_ENC_BUFFER_FORMAT[]> formats{
      new NV_ENC_BUFFER_FORMAT[count]};
  nverr = nvenc.nvEncGetInputFormats(encoder, NV_ENC_CODEC_HEVC_GUID,
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

  nverr = nvenc.nvEncInitializeEncoder(encoder, &init);
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

  nverr = nvenc.nvEncCreateInputBuffer(encoder, &input);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Error creating input buffer";
  }

  NV_ENC_CREATE_BITSTREAM_BUFFER output;
  memset(&output, 0, sizeof(output));

  output.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
  output.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
  output.size = init.encodeHeight * init.encodeWidth * 4;  // oversize

  nverr = nvenc.nvEncCreateBitstreamBuffer(encoder, &output);
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
  Frame frame(geometry.width * geometry.height * 1.5);

  for (frames_total = 0; video_y4m; frames_total++) {
    video_y4m >> frame;

    NV_ENC_LOCK_INPUT_BUFFER input_lock;
    memset(&input_lock, 0, sizeof(input_lock));

    input_lock.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
    input_lock.inputBuffer = input.inputBuffer;

    nverr = nvenc.nvEncLockInputBuffer(encoder, &input_lock);
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

    nverr = nvenc.nvEncUnlockInputBuffer(encoder, input_lock.inputBuffer);
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

    nverr = nvenc.nvEncEncodePicture(encoder, &encoding);
    if (nverr != NV_ENC_SUCCESS) {
      // Will not return NV_ENC_ERR_NEED_MORE_INPUT; no B-frames
      LOG(ERROR) << "Failed to encode encoding";
    }

    NV_ENC_LOCK_BITSTREAM output_lock;
    memset(&output_lock, 0, sizeof(output_lock));

    output_lock.version = NV_ENC_LOCK_BITSTREAM_VER;
    output_lock.outputBitstream = output.bitstreamBuffer;

    nverr = nvenc.nvEncLockBitstream(encoder, &output_lock);
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

    nverr = nvenc.nvEncUnlockBitstream(encoder, output_lock.outputBitstream);
    if (nverr != NV_ENC_SUCCESS) {
      LOG(ERROR) << "Failed to lock output bitstream";
    }
  }

  LOG(INFO) << "Encoded " << frames_total << " frames";
  LOG(INFO) << "With " << 100.f * packets_lost / packets_total
            << "% packet loss";

  nverr = nvenc.nvEncDestroyInputBuffer(encoder, input.inputBuffer);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to destroy input buffer";
  }

  nverr = nvenc.nvEncDestroyBitstreamBuffer(encoder, output.bitstreamBuffer);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to destroy input buffer";
  }

  nverr = nvenc.nvEncDestroyEncoder(encoder);
  if (nverr != NV_ENC_SUCCESS) {
    LOG(ERROR) << "Failed to destroy input buffer";
  }

  dlclose(nvenc_api);
  dlclose(cuda_driver_api);

  LOG(INFO) << "Thanks.";
}
