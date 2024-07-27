#include "lexer.hpp"

#include <iostream>
#include <ctype.h>

std::ostream& operator<<(std::ostream& stream, const Token& token) {
    switch (token.m_type) {
    case TokenType::CLAUSE: 
        stream << "CLAUSE";
        break;
    case TokenType::STRING:
        stream << "STRING : " << token.m_lexeme;
        break;
    case TokenType::IDENT:
        stream << "IDENT : " << token.m_lexeme;
        break;
    case TokenType::NUMBER:
        stream << "NUMBER : " << token.m_lexeme;
        break;
    case TokenType::SEMICOL:
        stream << "SEMICOL";
        break;
    case TokenType::INVALID:
        stream << "INVALID";
        break;
    case TokenType::LPAREN:
        stream << "LPAREN";
        break;
    case TokenType::RPAREN:
        stream << "RPAREN";
        break;
    case TokenType::COMMA:
        stream << "COMMA";
        break;
    }
    return stream;
}

const std::vector<Token>& Lexer::tokenize(std::ifstream& is) {
    is >> std::noskipws;

    char ch{};
    while (is >> ch) {
        std::string lexeme;
        TokenType tokenType;

        switch (ch) {
        case '\"':
            while ((is >> ch) && ch != '\"')
                lexeme += ch;
            tokenType = TokenType::STRING;
            break;
        case ';':
            lexeme += ";";
            tokenType = TokenType::SEMICOL;
            break;
        case '(':
            lexeme += "(";
            tokenType = TokenType::LPAREN;
            break;
        case ')':
            lexeme += ")";
            tokenType = TokenType::RPAREN;
            break;
        case ',':
            lexeme += ",";
            tokenType = TokenType::COMMA;
            break;
        case ' ':
            continue;
        default: {
            if (isdigit(ch)) {
                do {
                    lexeme += ch;
                } while ((is >> ch) && isdigit(ch));
                tokenType = TokenType::NUMBER;
            } else if (isalpha(ch)) {
                do {
                    lexeme += ch;
                } while ((is >> ch) && isalnum(ch));

                if (m_reserved.contains(lexeme)) {
                    tokenType = TokenType::CLAUSE;
                } else {
                    tokenType = TokenType::IDENT;
                }
            } else {
                // unrecognized characters are just ignored
                continue;
            }
        }
        }

        m_tokens.emplace_back(Token{lexeme, tokenType});
    }

    return m_tokens;
}
