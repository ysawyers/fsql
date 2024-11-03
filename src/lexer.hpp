#ifndef LEXER_HPP
#define LEXER_HPP

#include <fstream>
#include <vector>

namespace lexer 
{
    enum class TokenType 
    {
        SELECT,
        FILES,
        DIRECTORIES,
        ALL,

        LPAREN,
        RPAREN,
        COMMA,
        SEMICOL,

        STRING,
    };

    struct Token 
    {
        std::string m_lexeme;
        TokenType m_type;
    };

    void generate_tokens(std::istream& is, std::vector<Token>& tokens);
}

#endif
