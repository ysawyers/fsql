#ifndef AST_HPP
#define AST_HPP

#include <functional>
#include <filesystem>

#include "lexer.hpp"
#include "runtime_types.hpp"

struct Rule
{
    virtual void emit(std::vector<Instr>& program) = 0;
};

class AndRule : public Rule
{
    public:
        AndRule(std::shared_ptr<Rule> lhs, std::shared_ptr<Rule> rhs) : m_lhs(lhs), m_rhs(rhs) {};

        void emit(std::vector<Instr>& program);

    private:
        std::shared_ptr<Rule> m_lhs;
        std::shared_ptr<Rule> m_rhs;
};

class OrRule : public Rule
{
    public:
        OrRule(std::shared_ptr<Rule> lhs, std::shared_ptr<Rule> rhs) : m_lhs(lhs), m_rhs(rhs) {};

        void emit(std::vector<Instr>& program);
    
    private:
        std::shared_ptr<Rule> m_lhs;
        std::shared_ptr<Rule> m_rhs;
};

class ExtensionRule : public Rule
{
    public:
        ExtensionRule(const std::string& extension) : m_extension(extension) {};

        void emit(std::vector<Instr>& program);
    
    private:
        const std::string& m_extension;
};

class SizeRule : public Rule
{
    public:
        SizeRule();

        void emit(std::vector<Instr>& program);
};

struct Element
{
    virtual void emit(std::vector<Instr>& program) = 0;

    virtual bool is_atomic_element() = 0;
    virtual bool conflicting_select_type(lexer::TokenType parent_select_type) = 0;
};

class AtomicElement : public Element
{
    public:
        AtomicElement(const std::string& path);

        void emit(std::vector<Instr>& program);

        bool is_atomic_element() { return true; };
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
        bool conflicting_select_type(lexer::TokenType parent_select_type);

    public:
        std::vector<std::shared_ptr<Element>> m_elements;
        std::shared_ptr<Rule> m_rule;

    private:
        lexer::TokenType m_select_type;
};

struct DiskOperation
{
    virtual void emit(std::vector<Instr>& program) = 0;
};

class DisplayOp : public DiskOperation
{
    public:
        void emit(std::vector<Instr>& program);
};

class DeleteOp : public DiskOperation
{
    public:
        void emit(std::vector<Instr>& program);
};

class CopyOp : public DiskOperation
{
    public:
        CopyOp(const std::string& path);

        void emit(std::vector<Instr>& program);

    public:
        std::filesystem::path m_destination_path;
};

class MoveOp : public DiskOperation
{
    public:
        MoveOp(const std::string& path);

        void emit(std::vector<Instr>& program);

    public:
        std::filesystem::path m_destination_path;
};

class Query
{
    public:
        Query(lexer::TokenType select_type) : m_select_type(select_type) {};

        void emit(std::vector<Instr>& program);

    public:
        lexer::TokenType m_select_type;
        std::vector<std::shared_ptr<Element>> m_elements;
        std::shared_ptr<Rule> m_rule;
        std::shared_ptr<DiskOperation> m_disk_operation;
};

struct AST
{
    public:
        std::vector<Instr> compile();

        // removes compound elements from the AST that conflict with the parent's select specifier 
        void prune_conflicting_select();

    public:
        std::vector<std::shared_ptr<Query>> m_queries;
};

#endif
