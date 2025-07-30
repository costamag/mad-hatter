#include <iostream>
#include <replxx.hxx>

int main() {
  replxx::Replxx repl;
  std::string prompt = "> ";

  while (true) {
    char const* input = repl.input(prompt.c_str());
    if (input == nullptr) break;

    std::string line(input);
    repl.history_add(line);

    if (line == "quit" || line == "exit")
      break;

    // Process command
    std::cout << "You typed: " << line << std::endl;
  }

  return 0;
}