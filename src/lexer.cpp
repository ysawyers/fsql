#include "lexer.hpp"

#include <iostream>
#include <ctype.h>

std::ostream& operator<<(std::ostream& stream, const Token& token) {
    switch (token.m_type) {
    case TokenType::SELECT_CLAUSE: 
        stream << "SELECT_CLAUSE";
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
    case TokenType::MODIFYING_CLAUSE:
        stream << "MODIFYING_CLAUSE";
        break;
    case TokenType::FILTERING_CLAUSE:
        stream << "FILTERING_CLAUSE";
        break;
    case TokenType::SELECT_MODIFIER:
        stream << "SELECT_MODIFIER";
        break;
    }
    return stream;
}

const std::vector<Token>& Lexer::tokenize(std::ifstream& is) {
    is >> std::noskipws;

    char ch{};
    while (is >> ch) {
        std::string lexeme;
        TokenType token_type;

        switch (ch) {
        case '\"':
            while ((is >> ch) && ch != '\"')
                lexeme += ch;
            token_type = TokenType::STRING;
            break;
        case ';':
            lexeme += ";";
            token_type = TokenType::SEMICOL;
            break;
        case '(':
            lexeme += "(";
            token_type = TokenType::LPAREN;
            break;
        case ')':
            lexeme += ")";
            token_type = TokenType::RPAREN;
            break;
        case ',':
            lexeme += ",";
            token_type = TokenType::COMMA;
            break;
        case ' ':
            continue;
        default: {
            if (isdigit(ch)) {
                do {
                    lexeme += ch;
                } while (isdigit(is.peek()) && (is >> ch));
                token_type = TokenType::NUMBER;
            } else if (isalpha(ch)) {
                do {
                    lexeme += ch;
                } while (isalnum(is.peek()) && (is >> ch));

                if (lexeme == "SELECT") {
                    token_type = TokenType::SELECT_CLAUSE;
                } else if (m_modifying_clauses.contains(lexeme)) {
                    token_type = TokenType::MODIFYING_CLAUSE;
                } else if (m_filtering_clauses.contains(lexeme)) {
                    token_type = TokenType::FILTERING_CLAUSE;
                } else if (m_select_modifiers.contains(lexeme)) {
                    token_type = TokenType::SELECT_MODIFIER;
                } else {
                    token_type = TokenType::IDENT;
                }
            } else {
                // unrecognized characters are just ignored
                continue;
            }
        }
        }

        m_tokens.emplace_back(Token{lexeme, token_type});
    }

    return m_tokens;
}
