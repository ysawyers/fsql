#include "lexer.hpp"

#include <unordered_map>
#include <format>

namespace lexer
{
    std::unordered_map<std::string, TokenType> select_type =
    {
        {"files", TokenType::FILES},
        {"directories", TokenType::DIRECTORIES},
        {"all", TokenType::ALL},
        {"recursive", TokenType::RECURSIVE}
    };

    std::unordered_map<std::string, TokenType> disk_operations =
    {
        {"move", TokenType::MOVE},
        {"copy", TokenType::COPY},
        {"delete", TokenType::DELETE},
        {"display", TokenType::DISPLAY}
    };

    void handle_string(std::istream& is, std::string& lexeme)
    {
        char ch{};
        while ((is >> ch) && ch != '\"')
        {
            lexeme += ch;
        }
    }

    void generate_tokens(std::istream& is, std::vector<Token>& tokens) 
    {
        char ch{};

        while (is >> ch)
        {
            Token new_token;

            switch (ch) 
            {
            case '\"':
                handle_string(is, new_token.m_lexeme);
                new_token.m_type = TokenType::STRING;
                break;
            case ';':
                new_token.m_lexeme = ch;
                new_token.m_type = TokenType::SEMICOL;
                break;
            case '(':
                new_token.m_lexeme = ch;
                new_token.m_type = TokenType::LPAREN;
                break;
            case ')':
                new_token.m_lexeme = ch;
                new_token.m_type = TokenType::RPAREN;
                break;
            case ',':
                new_token.m_lexeme = ch;
                new_token.m_type = TokenType::COMMA;
                break;
            default:
                if (isalpha(ch)) 
                {
                    do 
                    {
                        new_token.m_lexeme += ch;
                    } while (isalnum(is.peek()) && (is >> ch));

                    if (new_token.m_lexeme == "select") 
                    {
                        new_token.m_type = TokenType::SELECT;
                    }
                    else if (select_type.contains(new_token.m_lexeme)) 
                    {
                        new_token.m_type = select_type[new_token.m_lexeme];
                    }
                    else if (disk_operations.contains(new_token.m_lexeme))
                    {
                        new_token.m_type = disk_operations[new_token.m_lexeme];
                    }
                    else
                    {
                        throw std::runtime_error(std::format("invalid token: {}", new_token.m_lexeme));
                    }
                }
                else 
                {
                    throw std::runtime_error("invalid token: ???");
                }
            }

            tokens.emplace_back(new_token);
        }
    }
}
