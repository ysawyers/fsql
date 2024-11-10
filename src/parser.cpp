#include "parser.hpp"

#include <iostream>

Parser::Parser(std::istream& is) : m_token_pos(0)
{
    lexer::generate_tokens(is, m_tokens);
}

lexer::Token& Parser::next_token() 
{
    return m_tokens[m_token_pos++];
}

void Parser::push_back_token() 
{
    if (m_token_pos > 0)
    {
        m_token_pos--;
    }
}

bool Parser::is_select_type(lexer::TokenType tokenType)
{
    return (tokenType == lexer::TokenType::FILES) || (tokenType == lexer::TokenType::DIRECTORIES) ||
        (tokenType == lexer::TokenType::ALL) || (tokenType == lexer::TokenType::RECURSIVE);
}

std::shared_ptr<Rule> Parser::primary_rule()
{
    switch (next_token().m_type)
    {
    case lexer::TokenType::EXTENSION:
        if (next_token().m_type == lexer::TokenType::EQ)
        {
            auto& string_tok = next_token();
            if (string_tok.m_type == lexer::TokenType::STRING)
            {
                return std::make_shared<ExtensionRule>(string_tok.m_lexeme);
            }
            throw std::runtime_error("invalid syntax: expected string");
        }
        throw std::runtime_error("invalid syntax: missing =");
    case lexer::TokenType::SIZE:
        {
            auto comparison_tok = next_token();
            if ((comparison_tok.m_type == lexer::TokenType::LTHAN) || (comparison_tok.m_type == lexer::TokenType::GTHAN))
            {
                auto threshold_tok = next_token();
                if (threshold_tok.m_type == lexer::TokenType::NUMBER)
                {
                    std::uint64_t threshold = std::stoi(threshold_tok.m_lexeme);
                    switch (next_token().m_type)
                    {
                    case lexer::TokenType::B: break;
                    case lexer::TokenType::KB:
                        threshold *= 1024;
                        break;
                    case lexer::TokenType::MB:
                        threshold *= 1024 * 1024;
                        break;
                    case lexer::TokenType::GB:
                        threshold *= 1024 * 1024 * 1024;
                        break;
                    default: throw std::runtime_error("invalid syntax: expected size type (B, KB, MB, or GB)");
                    }
                    return std::make_shared<SizeRule>(threshold, comparison_tok.m_type == lexer::TokenType::LTHAN);
                }
                throw std::runtime_error("invalid syntax: expected number");
            }
            throw std::runtime_error("invalid syntax: expected comparison operator");
        }
    case lexer::TokenType::LPAREN:
        {
            auto compound_rule = and_rule();
            if (next_token().m_type == lexer::TokenType::RPAREN)
            {
                return compound_rule;
            }
            throw std::runtime_error("invalid syntax: missing )");
        }
    default: throw std::runtime_error("invalid syntax: expected keyword after where");
    }
}

std::shared_ptr<Rule> Parser::or_rule()
{
    std::shared_ptr<Rule> lhs = primary_rule();
    if (next_token().m_type == lexer::TokenType::OR) 
    {
        std::shared_ptr<Rule> rhs = primary_rule();
        return std::make_shared<OrRule>(lhs, rhs);
    }
    else
    {
        push_back_token();
    }
    return lhs;
}

std::shared_ptr<Rule> Parser::and_rule()
{
    std::shared_ptr<Rule> lhs = or_rule();
    if (next_token().m_type == lexer::TokenType::AND)
    {
        std::shared_ptr<Rule> rhs = or_rule();
        return std::make_shared<AndRule>(lhs, rhs);
    }
    else
    {
        push_back_token();
    }
    return lhs;
}

std::shared_ptr<DiskOperation> Parser::disk_operation()
{
    switch (next_token().m_type)
    {
    case lexer::TokenType::DISPLAY: return std::make_shared<DisplayOp>();
    case lexer::TokenType::DELETE: return std::make_shared<DeleteOp>();
    case lexer::TokenType::COPY:
    {
        auto& destination_path = next_token();
        if (destination_path.m_type == lexer::TokenType::STRING)
        {
            return std::make_shared<CopyOp>(destination_path.m_lexeme);
        }
        throw std::runtime_error("invalid syntax: expected string");
    }
    case lexer::TokenType::MOVE:
    {
        auto& destination_path = next_token();
        if (destination_path.m_type == lexer::TokenType::STRING)
        {
            return std::make_shared<MoveOp>(destination_path.m_lexeme);
        }
        throw std::runtime_error("invalid syntax: expected string");
    }
    default: throw std::runtime_error("invalid syntax: expected operation keyword");
    }
}

std::shared_ptr<CompoundElement> Parser::compound_element()
{
    if (next_token().m_type == lexer::TokenType::SELECT)
    {
        lexer::TokenType select_type;
        if (is_select_type(select_type = next_token().m_type))
        {
            std::shared_ptr<CompoundElement> compound_element = std::make_shared<CompoundElement>(select_type);
            compound_element->m_elements = element_list();

            if (next_token().m_type == lexer::TokenType::WHERE)
            {
                compound_element->m_rule = and_rule();
            }
            else
            {
                push_back_token();
            }
            if (next_token().m_type == lexer::TokenType::RPAREN)
            {
                return compound_element;
            }

            throw std::runtime_error("invalid syntax: missing )");
        }
        throw std::runtime_error("invalid syntax: expected select specifier");
    }
    throw std::runtime_error("invalid syntax: expected select keyword");
}

std::shared_ptr<Element> Parser::element()
{
    auto& tok = next_token();
    if (tok.m_type == lexer::TokenType::STRING)
    {
        return std::make_shared<AtomicElement>(tok.m_lexeme);
    }
    else if (tok.m_type == lexer::TokenType::LPAREN)
    {
        return compound_element();
    }
    throw std::runtime_error("invalid syntax: expected string or sub-query");
}

std::vector<std::shared_ptr<Element>> Parser::element_list()
{
    std::vector<std::shared_ptr<Element>> elements;
    do
    {
        elements.emplace_back(element());
    } while (next_token().m_type == lexer::TokenType::COMMA);
    push_back_token();
    return elements;
}

std::shared_ptr<Query> Parser::query()
{
    if (next_token().m_type == lexer::TokenType::SELECT)
    {
        lexer::TokenType select_type;
        if (is_select_type(select_type = next_token().m_type))
        {
            std::shared_ptr<Query> query = std::make_shared<Query>(select_type);
            query->m_elements = element_list();

            if (next_token().m_type == lexer::TokenType::WHERE)
            {
                query->m_rule = and_rule();
            }
            else
            {
                push_back_token();
            }

            query->m_disk_operation = disk_operation();

            if (next_token().m_type == lexer::TokenType::SEMICOL)
            {
                return query;
            }

            throw std::runtime_error("invalid syntax: missing semicolon");
        }
        throw std::runtime_error("invalid syntax: expected select specifier");
    }
    throw std::runtime_error("invalid syntax: expected select keyword");
}

std::unique_ptr<AST> Parser::build_ast() 
{
    std::unique_ptr<AST> ast = std::make_unique<AST>();
    while (next_token().m_type != lexer::TokenType::DONE)
    {
        push_back_token();
        ast->m_queries.emplace_back(query());
    }
    return ast;
}
