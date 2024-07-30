#ifndef LEXER_HPP
#define LEXER_HPP

#include <fstream>
#include <vector>
#include <set>
#include <string>

enum class TokenType {
    SELECT_CLAUSE,
    MODIFYING_CLAUSE,

    IDENT,
    STRING,
    NUMBER,
    SEMICOL,
    LPAREN,
    RPAREN,
    COMMA,
    INVALID
};

struct Token {
    std::string m_lexeme;
    TokenType m_type;
};

std::ostream& operator<<(std::ostream& stream, const Token& token);

class Lexer {
    public:

        const std::vector<Token>& tokenize(std::ifstream& is);

    private:

        std::vector<Token> m_tokens;
        std::set<std::string> m_modifying_clauses{"MOVE", "DELETE", "COPY"};
};

#endif
