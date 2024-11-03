#ifndef AST_HPP
#define AST_HPP

#include <functional>
#include <filesystem>

#include "lexer.hpp"
#include "runtime_types.hpp"

struct Element
{
    virtual void emit(std::vector<Instr>& program) = 0;

    virtual bool is_atomic_element() = 0;
    virtual bool contains_invalid_paths() = 0;
    virtual bool conflicting_select_type(lexer::TokenType parent_select_type) = 0;
};

class AtomicElement : public Element
{
    public:
        AtomicElement(std::string& path);

        void emit(std::vector<Instr>& program);

        bool is_atomic_element() { return true; };
        bool contains_invalid_paths();
        bool conflicting_select_type(lexer::TokenType parent_select_type);

    private:
        std::filesystem::path m_path;
};

class CompoundElement : public Element
{
    public:
        CompoundElement(lexer::TokenType select_type) : m_select_type(select_type) {};

        void emit(std::vector<Instr>& program);

        bool is_atomic_element() { return false; };
        bool contains_invalid_paths();
        bool conflicting_select_type(lexer::TokenType parent_select_type);

    public:
        std::vector<std::shared_ptr<Element>> m_elements;

    private:
        lexer::TokenType m_select_type;
};

class Query
{
    public:
        Query(lexer::TokenType select_type) : m_select_type(select_type) {};

        void emit(std::vector<Instr>& program);

    public:
        lexer::TokenType m_select_type;
        std::vector<std::shared_ptr<Element>> m_elements;
};

struct AST
{
    public:
        std::vector<Instr> compile();

        // removes paths that do not exist from the AST
        void path_validation();

        // removes compound elements from the AST that conflict with the parent's select specifier 
        void prune_conflicting_select();

    public:
        std::vector<std::shared_ptr<Query>> m_queries;
};

#endif
