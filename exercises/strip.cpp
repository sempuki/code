#include <string>
#include <algorithm>

std::string rstrip(std::string input, const std::string& to_strip) {
  input.erase(std::find_first_of(input.rbegin(), input.rend(), to_strip.begin(),
                                 to_strip.end(),
                                 [](char c, char x) { return c != x; }).base(),
              input.end());
  return input;
}

std::string lstrip(std::string input, const std::string& to_strip) {
  input.erase(input.begin(),
              std::find_first_of(input.begin(), input.end(), to_strip.begin(),
                                 to_strip.end(),
                                 [](char c, char x) { return c != x; }));
  return input;
}

std::string strip(std::string input, const std::string& to_strip) {
  return lstrip(rstrip(std::move(input), to_strip), to_strip);
}
