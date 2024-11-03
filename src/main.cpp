#include <iostream>

#include "parser.hpp"
#include "runtime.hpp"

const std::string program = "FSQL 0.0.0";

void run(std::istream& stream) 
{
    try
    {
        Parser parser(stream);

        auto ast = parser.build_ast();
        ast->path_validation();
        ast->prune_conflicting_select();

        Runtime runtime;
        runtime.run(ast->compile());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

int main()
{
    std::ifstream source_file("source.fsql");
    run(source_file);
    return EXIT_SUCCESS;
}
