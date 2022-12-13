#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
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
  stream_buffer->clear();

  std::string buffer;
  if (std::getline(input, buffer)) {
    stream_buffer->str(std::move(buffer));
  }

  return input && stream_buffer->str().size() <= max_input_chars;
}

char next_quote(std::istream& input) {
  char delimiter = ' ';
  switch (input.peek()) {
    case '\"':
    case '\'': input.get(delimiter); break;
    default: break;
  }
  return delimiter;
}

bool is_balanced_quote(std::istream& input, char delimiter) {
  bool balanced = true;
  switch (delimiter) {
    case '\"':
    case '\'': {
      auto position = input.tellg();
      input.ignore(std::numeric_limits<std::streamsize>::max(), delimiter);
      balanced = !input.eof();
      input.seekg(position);
    } break;
    default: break;
  }
  return balanced;
}

bool extract_arguments(std::istream& input, std::vector<std::string>* args) {
  args->resize(0);

  std::string buffer;
  for (char delim = next_quote(input);
       is_balanced_quote(input, delim) && std::getline(input, buffer, delim);
       delim = next_quote(input)) {
    if (buffer.size()) {
      args->push_back(std::move(buffer));
    }
  }

  return input.eof() && args->size() <= max_input_args;
}

bool do_builtin_exit(const std::vector<std::string>& args, bool* quit) {
  if (args.size() == 1) {
    *quit = true;
    return true;
  }
  return false;
}

bool do_builtin_cd(const std::vector<std::string>& args) {
  if (args.size() == 2) {
    return ::chdir(args[1].c_str()) != -1;
  }
  return false;
}

bool do_builtin_pwd(const std::vector<std::string>& args, std::ostream& out) {
  if (args.size() == 1) {
    const char* cwd = ::get_current_dir_name();
    if (cwd != nullptr) {
      out << cwd << "\n";
      std::free((void*)cwd);
      return true;
    }
  }
  return false;
}

bool do_external_exec(const std::vector<std::string>& args, int* exit_status) {
  if (args.size() > 0) {
    switch (::fork()) {
      case -1:  // Error
        break;
      case 0:  // is child.
      {
        auto copy = args;  // exec*() wants potentially mutable chars.
        auto argv = std::vector<char*>(copy.size() + 1, nullptr);  // Trailing nullptr is required.
        std::transform(copy.begin(), copy.end(), argv.data(), [](auto& s) { return s.data(); });
        if (::execvp(argv[0], argv.data()) == -1) {
          std::cerr << "exec failed: " << std::strerror(errno) << std::endl;
          std::exit(errno);
        }
      } break;
      default:  // is parent.
        return ::wait(exit_status) != -1;
    }
  }
  return false;
}

}  // namespace

int main() {
  bool quit = false;
  std::vector<std::string> arguments;
  std::stringstream buffer;

  std::string prompt = "$ ";
  std::map<std::string, std::function<bool(const std::vector<std::string>&)>> builtins{
    {"exit",
     [&quit](auto&& args) {
       return do_builtin_exit(args, &quit);
     }},
    {"cd",
     [](auto&& args) {
       return do_builtin_cd(args);
     }},
    {"pwd",
     [](auto&& args) {
       return do_builtin_pwd(args, std::cerr);
     }},
  };

  while (!quit &&                              //
         present_prompt(std::cerr, prompt) &&  //
         extract_buffered_input(std::cin, &buffer)) {
    if (extract_arguments(buffer, &arguments)) {
      if (arguments.size()) {
        auto&& command = arguments.front();
        if (builtins.count(command)) {
          if (!builtins[command](arguments)) {
            std::cerr << "error: builtin " << command << " failed.";
          }
        } else {
          int exit_status = -1;
          if (!do_external_exec(arguments, &exit_status) || exit_status != EXIT_SUCCESS) {
            std::cerr << "error: " << command << " failed with status " << WEXITSTATUS(exit_status)
                      << std::endl;
          }
        }
      }
    } else {
      std::cerr << "error: failed to extract arguments." << std::endl;
    }
  }

  std::cerr << "Exiting shell.\n";
  if (!std::cin && !std::cin.eof()) {
    std::cerr << "There was an input error.\n";
    return -1;
  }
  return 0;
}
