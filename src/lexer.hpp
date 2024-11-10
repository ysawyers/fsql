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
        RECURSIVE,

        MOVE,
        COPY,
        DELETE,
        DISPLAY,

        WHERE,
        AND,
        OR,
        EXTENSION,
        SIZE,

        B,
        KB,
        MB,
        GB,

        LTHAN,
        GTHAN,
        LPAREN,
        RPAREN,
        COMMA,
        SEMICOL,
        EQ,

        STRING,
        NUMBER,

        DONE
    };

    struct Token 
    {
        std::string m_lexeme;
        TokenType m_type;
    };

    void generate_tokens(std::istream& is, std::vector<Token>& tokens);
}

#endif
