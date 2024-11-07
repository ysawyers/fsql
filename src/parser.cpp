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

bool Parser::has_next_token() 
{
    return m_token_pos < m_tokens.size();
};

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

std::shared_ptr<DiskOperation> Parser::disk_operation()
{
    switch (next_token().m_type)
    {
    case lexer::TokenType::DISPLAY: return std::make_shared<DisplayOp>();
    case lexer::TokenType::DELETE: return std::make_shared<DeleteOp>();
    case lexer::TokenType::COPY:
    {
        if (!has_next_token())
        {
            throw std::runtime_error("expected destination path after copy keyword");
        }
        auto& destination_path = next_token();
        if (destination_path.m_type == lexer::TokenType::STRING)
        {
            return std::make_shared<CopyOp>(destination_path.m_lexeme);
        }
        throw std::runtime_error("expected destination path after copy keyword");
    }
    case lexer::TokenType::MOVE:
    {
        if (!has_next_token())
        {
            throw std::runtime_error("expected destination path after move keyword");
        }
        auto& destination_path = next_token();
        if (destination_path.m_type == lexer::TokenType::STRING)
        {
            return std::make_shared<MoveOp>(destination_path.m_lexeme);
        }
        throw std::runtime_error("expected destination path after move keyword");
    }
    default: throw std::runtime_error("expected disk operation: display, delete, copy, move");
    }
}

std::shared_ptr<CompoundElement> Parser::compound_element()
{
    if (has_next_token() && (next_token().m_type == lexer::TokenType::SELECT))
    {
        lexer::TokenType select_type;
        if (has_next_token() && is_select_type(select_type = next_token().m_type))
        {
            std::shared_ptr<CompoundElement> compound_element = std::make_shared<CompoundElement>(select_type);
            do
            {
                compound_element->m_elements.emplace_back(element());
            } while (has_next_token() && (next_token().m_type == lexer::TokenType::COMMA));

            push_back_token();

            if (next_token().m_type == lexer::TokenType::RPAREN)
            {
                return compound_element;
            }
            throw std::runtime_error("expected closing )");
        }
        throw std::runtime_error("expected select specifier");
    }
    throw std::runtime_error("expected keyword: select");
}

std::shared_ptr<Element> Parser::element()
{
    if (has_next_token())
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
    }
    throw std::runtime_error("expected element");
}

std::shared_ptr<Query> Parser::query()
{
    if (next_token().m_type == lexer::TokenType::SELECT)
    {
        lexer::TokenType select_type;
        if (has_next_token() && is_select_type(select_type = next_token().m_type))
        {
            std::shared_ptr<Query> query = std::make_shared<Query>(select_type);
            do
            {
                query->m_elements.emplace_back(element());
            } while (has_next_token() && (next_token().m_type == lexer::TokenType::COMMA));
            push_back_token();

            query->m_disk_operation = disk_operation();

            if (next_token().m_type == lexer::TokenType::SEMICOL)
            {
                return query;
            }
            throw std::runtime_error("expected semicolon");
        }
        throw std::runtime_error("expected select specifier");
    }
    throw std::runtime_error("expected keyword: select");
}

std::unique_ptr<AST> Parser::build_ast() 
{
    std::unique_ptr<AST> ast = std::make_unique<AST>();
    while (has_next_token())
    {
        ast->m_queries.emplace_back(query());
    }
    return ast;
}
