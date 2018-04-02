#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>
#include <limits>
#include <memory>
#include <random>

#include <dlfcn.h>

#include "glog/logging.h"

namespace {

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

class DynamicLibrary {
 public:
  class Symbol {
   public:
    Symbol(DynamicLibrary& library, const std::string& name)
        : symbol_{dlsym(library.handle_, name.c_str())} {}

    Symbol(const Symbol&) = default;
    Symbol& operator=(const Symbol&) = default;

    Symbol(Symbol&&) = default;
    Symbol& operator=(Symbol&&) = default;

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

    Return operator()(Arguments... arguments) {
      return reinterpret_cast<Return (*)(Arguments...)>(this->symbol_)(
          std::forward<Arguments>(arguments)...);
    }
  };

  explicit DynamicLibrary(const std::string& name)
      : handle_{dlopen(name.c_str(), RTLD_LAZY | RTLD_LOCAL)} {}

  ~DynamicLibrary() {
    if (handle_) {
      dlclose(handle_);
    }
  }

  DynamicLibrary(const DynamicLibrary&) = delete;
  DynamicLibrary& operator=(const DynamicLibrary&) = delete;

  DynamicLibrary(DynamicLibrary&&) = default;
  DynamicLibrary& operator=(DynamicLibrary&&) = default;

  explicit operator bool() const { return handle_; }

  const char* error() const { return dlerror(); }

 private:
  void* handle_ = nullptr;
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

  std::ofstream outfile{argv[2]};
  if (!outfile) {
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

    outfile.write(reinterpret_cast<const char*>(output_lock.bitstreamBufferPtr),
                  output_lock.bitstreamSizeInBytes);
  }

  LOG(INFO) << "Encoded " << frames_total << " frames";
  LOG(INFO) << "With " << 100.f * packets_lost / packets_total
            << "% packet loss";
}
