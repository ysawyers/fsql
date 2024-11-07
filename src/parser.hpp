#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast.hpp"

class Parser
{
    public:
        Parser(std::istream& is);

        std::unique_ptr<AST> build_ast();

    private:
        lexer::Token& next_token();
        void push_back_token();
        bool has_next_token();

        std::shared_ptr<Query> query();
        std::shared_ptr<Element> element();
        std::shared_ptr<CompoundElement> compound_element();
        std::shared_ptr<DiskOperation> disk_operation();

        bool is_select_type(lexer::TokenType tokenType);

    private:
        std::vector<lexer::Token> m_tokens;
        std::uint32_t m_token_pos;
};

#endif
