#include <iostream>

#include "parser.hpp"
#include "runtime.hpp"

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
        return EXIT_SUCCESS;
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
        // TODO: interactive shell?
        exit(1);
    }
    else
    {
        std::ifstream source_file(argv[1]);
        if (source_file.fail())
        {
            std::cout << "failed to open: " << argv[1] << "\n";
            return EXIT_FAILURE;
        }
        return run(source_file);
    }
    return EXIT_SUCCESS;
}
