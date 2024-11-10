#include <iostream>

#include "parser.hpp"
#include "runtime.hpp"

#include <iostream>
#include <string>

const char* program = "FSQL 0.0.0";

int run(std::istream& stream) 
{
    try
    {
        Parser parser(stream);

        auto ast = parser.build_ast();
        ast->prune_conflicting_select();

        Runtime runtime;
        runtime.run(ast->compile());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        // TODO
        exit(1);
    }
    else
    {
        std::ifstream source_file(argv[1]);
        return run(source_file);
    }
    return EXIT_SUCCESS;
}
