#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "runtime.hpp"

class Parser {
    public:

        Parser(const std::vector<Token>& tokens, std::vector<Instr>& program) 
            : m_curr_token_idx(0), m_tokens(tokens), m_program(program) {};

        void generate_program();

    private:

        //! STMT = clause ";"
        bool statement();

        //! CLAUSE = select_clause | action_clause
        bool clause();

        //! SELECT_CLAUSE = virtual_directory, { virtual_directory }, { modifying_clause }
        bool select_clause();

        //! MODIFYING_CLAUSE = ...
        bool modifying_clause();

        //! ACTION_CLAUSE = ...
        bool action_clause();

        //! VIRTUAL_DIRECTORY = string | "(" { select_clause } ")"
        bool virtual_directory(bool& is_disk_element);

        const Token& next_token();

        void push_back_token();

        bool has_next_token();

        std::vector<Instr>& m_program;
        const std::vector<Token>& m_tokens;
        std::size_t m_curr_token_idx;
};

#endif
