#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

struct Token {
  Token() = default;
  Token(int value, int triplet, int position)
    : value{value}, triplet{triplet}, position{position} {}
  int value = -1, triplet = -1, position = -1;
};

const std::vector<std::string> triplet_name = {
  "", "thousand", "million", "billion"
};

const std::vector<std::string> tens_value_name = {
  "", "ten", "twenty", "thirty", "fourty", "fifty", "sixty", "seventy", "eighty", "ninety"
};

const std::vector<std::string> ones_value_name = {
  "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
  "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
};

std::vector<Token> parse(int value) {
  std::vector<Token> result;
  int count = 0;
  while (value) {
    result.push_back({value % 10, count / 3, count % 3});
    value /= 10;
    count += 1;
  }
  std::reverse(begin(result), end(result));
  return result;
}

std::string format(const std::vector<Token> &tokens) {
  std::stringstream result;

  Token previous;
  for (auto &&token : tokens) {
    if (previous.triplet >= 0) {
      result << " ";
      if (token.triplet != previous.triplet) {
        result << triplet_name[previous.triplet] << ", ";
      }
    }
    switch (token.position) {
      case 2:
        result << ones_value_name[token.value] << " hundred";
        break;
      case 1:
        result << (previous.position == 2 ? "and " : "") << (token.value > 1 ? tens_value_name[token.value] : "");
        break;
      case 0:
        result << ones_value_name[token.value + ((previous.position == 1 && previous.value == 1) ? 10 : 0)];
        break;
    }
    previous = token;
  }

  return result.str();
}

std::string int_to_english(int value) {
  return format(parse(value));
}

int main() {
  std::cout << int_to_english(1234567890) << std::endl;
}
