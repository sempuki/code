#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace {
constexpr size_t max_input_chars = 1000;
constexpr size_t max_input_args = 100;

bool present_prompt(std::ostream& output, const std::string& prompt) {
  output << prompt << std::flush;
  return static_cast<bool>(output);
}

bool extract_buffered_input(std::istream& input, std::stringstream* stream_buffer) {
  std::string buffer;
  if (std::getline(input, buffer)) {
    stream_buffer->clear();
    stream_buffer->str(std::move(buffer));
  }
  return input && stream_buffer->str().size() <= max_input_chars;
}

bool extract_arguments(std::istream& input, std::vector<std::string>* args) {
  args->resize(0);
  std::string buffer;
  while (std::getline(input, buffer, ' ')) {
    if (buffer.size()) {
      args->push_back(std::move(buffer));
    }
  }
  return input.eof() && args->size() && args->size() <= max_input_args;
}

bool extract_path(std::vector<std::string>* paths) {
  const char* env_path = std::getenv("PATH");
  const size_t env_path_size = std::strlen(env_path);

  std::stringstream input;
  input.write(env_path, static_cast<std::streamsize>(env_path_size));

  paths->resize(0);
  std::string buffer;
  while (std::getline(input, buffer, ':')) {
    if (buffer.size()) {
      paths->push_back(std::move(buffer));
    }
  }

  return input.eof();
}

bool do_builtin_exit(const std::vector<std::string>& args, bool* quit) {
  if (args.size() == 1) {
    *quit = true;
    return true;
  }
  return false;
}

bool do_builtin_cd(const std::vector<std::string>& args, std::string* cwd) {
  if (args.size() == 2) {
    *cwd = args[1];
    return true;
  }
  return false;
}

bool do_builtin_pwd(const std::vector<std::string>& args,
                    const std::string& cwd,
                    std::ostream& out) {
  if (args.size() == 1) {
    out << cwd << "\n";
    return true;
  }
  return false;
}
}  // namespace

int main() {
  bool quit = false;
  std::string curr_working_dir;
  std::vector<std::string> paths;
  std::vector<std::string> arguments;
  std::stringstream buffer;

  std::string prompt = "$ ";
  std::map<std::string, std::function<bool(const std::vector<std::string>&)>> builtins{
    {"exit",
     [&quit](auto&& args) {
       return do_builtin_exit(args, &quit);
     }},
    {"cd",
     [&curr_working_dir](auto&& args) {
       return do_builtin_cd(args, &curr_working_dir);
     }},
    {"pwd",
     [&curr_working_dir](auto&& args) {
       return do_builtin_pwd(args, curr_working_dir, std::cout);
     }},
  };

  if (extract_path(&paths)) {
    while (!quit &&                              //
           present_prompt(std::cout, prompt) &&  //
           extract_buffered_input(std::cin, &buffer)) {
      if (extract_arguments(buffer, &arguments)) {
        auto&& command = arguments.front();
        if (builtins.count(command)) {
          if (!builtins[command](arguments)) {
            std::cerr << "error: " << command << " failed.";
          }
        } else {
        }
      }
    }
  }

  std::cerr << "Exiting shell.\n";
  if (!std::cin && !std::cin.eof()) {
    std::cerr << "There was an input error.\n";
    return -1;
  }
  return 0;
}
