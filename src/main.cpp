#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>
#include <sstream>

const std::string_view program = "FSQL 0.0.1";

void run(std::istringstream& stream) {
    Lexer lexer;
    const auto& tokens = lexer.tokenize(stream);
    
    Runtime runtime;
    Parser parser(tokens, runtime.m_program);
    if (parser.generate_program()) runtime.execute_program();
}

int main() {
    std::cout << program << "\n";
    while (true) {
        printf(">>> ");

        std::string query;
        std::getline(std::cin, query, '\n');
        if (!query.empty()) {
            std::istringstream stream(query);
            run(stream);
        }
    }

    return EXIT_SUCCESS;
}
