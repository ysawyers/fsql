#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "runtime.hpp"

class Parser {
    public:

        Parser(const std::vector<Token>& tokens, std::vector<Instr>& program) 
            : m_curr_token_idx(0), m_tokens(tokens), m_program(program), m_has_modifying_clause(false) {};

        bool generate_program();

    private:

        //! STMT = select_clause, [ modifying_clause ] ";"
        bool statement();

        //! SELECT_CLAUSE = "SELECT" [ "FILES" | "DIRECTORIES" ], virtual_directory, { virtual_directory }, { filtering_clause }
        bool select_clause();

        //! MODIFYING_CLAUSE = COPY string_literal | MOVE string_literal | DELETE
        bool modifying_clause();

        //! FILTERING_CLAUSE = ???
        bool filtering_clause();

        //! VIRTUAL_DIRECTORY = string_literal | "(" select_clause ")"
        bool virtual_directory(bool& is_disk_element);

        const Token& next_token();

        void push_back_token();

        bool has_next_token();

        std::vector<Instr>& m_program;
        bool m_has_modifying_clause;
        const std::vector<Token>& m_tokens;
        std::size_t m_curr_token_idx;
};

#endif
