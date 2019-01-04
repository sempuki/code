#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {
class BitStream {
 public:
  BitStream(const uint8_t* base, uint32_t bits) : base_{base}, end_bit_{bits} {}

  BitStream(const BitStream&) = default;
  BitStream& operator=(const BitStream&) = default;

  BitStream(BitStream&&) = default;
  BitStream& operator=(BitStream&&) = default;

  BitStream() = default;
  ~BitStream() = default;

  template <typename Output>
  uint32_t read(uint32_t bits, Output& output) {
    uint32_t total = 0;
    uint32_t curr_bit = pos_bit_;
    uint32_t term_bit = pos_bit_ + bits;

    while (curr_bit < term_bit && term_bit <= end_bit_) {
      auto offset_byte = to_offset_byte(curr_bit);
      auto offset_bit = to_offset_bit(curr_bit);
      auto count = std::min(bits - total, 8 - offset_bit);
      uint8_t data = base_[offset_byte];

      data <<= offset_bit;
      data >>= 8 - count;

      output <<= count;
      output |= data;

      curr_bit += count;
      total += count;
    }

    pos_bit_ += total;
    return total;
  }

  bool read_bit() {
    auto curr_bit = pos_bit_++;
    return (base_[to_offset_byte(curr_bit)] << to_offset_bit(curr_bit)) & 0x80;
  }

  uint32_t seek(uint32_t bits) {
    uint32_t count = std::min(bits, end_bit_ - pos_bit_);
    pos_bit_ += count;
    return count;
  }

  uint32_t tell() const {
    return pos_bit_;
  }

 private:
  uint32_t to_offset_byte(uint32_t bit) {
    return bit >> 3;
  }

  uint32_t to_offset_bit(uint32_t bit) {
    return bit & 0x7;
  }

  const uint8_t* base_ = nullptr;
  uint32_t pos_bit_ = 0;
  uint32_t end_bit_ = 0;
};

BitStream build_bitstream(const std::vector<uint8_t>& bytes) {
  return BitStream(bytes.data(), bytes.size() * 8);
}

struct ParseMap {
  struct Field {
    enum class Type : uint8_t { unknown, fixed, variable_exp_golomb };

    Field() = default;

    Field(Type type) : type{type} {}
    Field(Type type, uint8_t width) : bit_width{width}, type{type} {}

    Field(const Field&) = default;
    Field& operator=(const Field&) = default;

    Field(Field&&) = default;
    Field& operator=(Field&&) = default;

    uint64_t bit_pattern = 0;
    uint16_t bit_position = 0;
    uint8_t bit_width = 0;
    Type type = Type::unknown;
    bool valid = false;
  };

  std::map<std::string, Field> fields;
};

ParseMap build_sps_parse_map() {
  ParseMap map;

  map.fields["sps_video_paramter_set_id"] = {ParseMap::Field::Type::fixed, 4};
  map.fields["sps_max_sub_layers_minus1"] = {ParseMap::Field::Type::fixed, 3};

  return map;
}

bool try_parse_fixed_width(BitStream& stream, ParseMap::Field& field) {
  uint32_t expect_count = field.bit_width;

  field.bit_pattern = 0;
  field.bit_position = stream.tell();
  field.bit_width = stream.read(field.bit_width, field.bit_pattern);
  field.valid = field.bit_width == expect_count;

  return field.valid;
}

bool try_parse_variable_exp_golomb(BitStream& stream, ParseMap::Field& field) {
  uint32_t zero_count = 0;

  field.bit_pattern = 0;
  field.bit_position = stream.tell();

  for (; !stream.read_bit(); zero_count++)
    ;

  field.bit_width = stream.read(zero_count, field.bit_pattern) + zero_count + 1;
  field.bit_pattern += ((1 << zero_count) - 1);
  field.valid = field.bit_width == (2 * zero_count + 1);

  return field.valid;
}

bool try_parse(const std::string& name, BitStream& stream, ParseMap& map) {
  auto iter = map.fields.find(name);
  if (iter != map.fields.end()) {
    switch (iter->second.type) {
      case ParseMap::Field::Type::fixed:
        return try_parse_fixed_width(stream, iter->second);

      case ParseMap::Field::Type::variable_exp_golomb:
        return try_parse_variable_exp_golomb(stream, iter->second);

      default:
        break;
    }
  }

  return false;
}

ParseMap parse_sps(const std::vector<uint8_t>& rbsp) {
  auto stream = build_bitstream(rbsp);
  auto map = build_sps_parse_map();

  try_parse("sps_video_paramter_set_id", stream, map);
  try_parse("sps_max_sub_layers_minus1", stream, map);

  return map;
}

void log_parsed_field(const std::string& name, const ParseMap::Field& field) {
  if (field.valid) {
    std::cout << "-- [" << field.bit_position << "] " << name << " : "
              << field.bit_pattern << " ("
              << static_cast<size_t>(field.bit_width) << ")\n";
  }
}

void log_parsed_map(const ParseMap& map) {
  for (auto&& pair : map.fields) {
    log_parsed_field(pair.first, pair.second);
  }
}

} // namespace

std::vector<uint8_t> sps_rbsp_a{
    0x1,  0x1, 0x40, 0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x96,
    0xa0, 0x3, 0xc0, 0x80, 0x13, 0x7, 0xc4, 0xe5, 0x89, 0xd2, 0x90, 0x94, 0x4b,
    0xe4, 0x2, 0x0,  0x0,  0x0,  0x2, 0x0,  0x0,  0x0,  0x28, 0x10,
};

std::vector<uint8_t> sps_rbsp_b{
    0x1,  0x1, 0x40, 0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x96,
    0xa0, 0x3, 0xd0, 0x80, 0x13, 0x7, 0x1f, 0x3e, 0x58, 0x9e, 0xe4, 0x25, 0x12,
    0xf9, 0x0, 0x80, 0x0,  0x0,  0x0, 0x80, 0x0,  0x0,  0xa,  0x4};

std::vector<uint8_t> sps_rbsp_c{
    0x1,  0x1, 0x40, 0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x96,
    0xa0, 0x3, 0xc0, 0x80, 0x13, 0x7, 0xc4, 0xe5, 0x89, 0xd2, 0x90, 0x96, 0x4b,
    0xe4, 0x2, 0x0,  0x0,  0x0,  0x2, 0x0,  0x0,  0x0,  0x28, 0x10};

int main(int argc, char** argv) {
  log_parsed_map(parse_sps(sps_rbsp_a));
}
