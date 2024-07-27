#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>

int main() {
    std::ifstream file("example.txt");
    Lexer lexer;

    const auto& tokens = lexer.tokenize(file);
    std::vector<Instr> program;
    Parser parser(tokens, program);

    try {
        parser.generate_program();
        Runtime runtime;
        runtime.execute_program(program);
    } catch (...) {
        std::cout << "we failed!\n";
    }

    return EXIT_SUCCESS;
}
