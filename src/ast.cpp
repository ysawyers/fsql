#include "ast.hpp"

#include <iostream>
#include <format>

namespace fs = std::filesystem;

fs::path format_path(const std::string& path)
{
    if (!path.size())
    {
        throw std::runtime_error("compilation error: cannot specify an empty path");
    }

    if (path.substr(0, 2) == "~/")
    {
        return fs::path(getenv("HOME")) / fs::path(path.substr(2));
    }
    else if (fs::exists(path))
    {
        return std::filesystem::canonical(path);
    }
    else
    {
        throw std::runtime_error(std::format("compilation error: {} does not exist", path));
    }
}

std::uint64_t select_specifier(lexer::TokenType select_type)
{
    switch (select_type)
    {
    case lexer::TokenType::ALL: return 0b11;
    case lexer::TokenType::FILES: return 0b01;
    case lexer::TokenType::DIRECTORIES: return 0b10;
    case lexer::TokenType::RECURSIVE: return 0b00;
    default: std::unreachable();
    }
}

AtomicElement::AtomicElement(const std::string& path)
{
    m_path = format_path(path);
}

bool AtomicElement::conflicting_select_type(lexer::TokenType parent_select_type)
{
    return (parent_select_type == lexer::TokenType::DIRECTORIES) && fs::is_regular_file(m_path);
}

void AtomicElement::emit(std::vector<Instr>& program)
{
    program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(&m_path) });
}

bool CompoundElement::conflicting_select_type(lexer::TokenType parent_select_type)
{
    bool remaining_children = false;
    for (auto& element : m_elements)
    {
        if (element && element->conflicting_select_type(m_select_type))
        {
            element = nullptr;
        }
        else
        {
            remaining_children = true;
        }
    }
    return !remaining_children || ((parent_select_type == lexer::TokenType::DIRECTORIES) && 
        ((m_select_type == lexer::TokenType::FILES) || (m_select_type == lexer::TokenType::RECURSIVE)));
}

void CompoundElement::emit(std::vector<Instr>& program)
{
    std::uint64_t n_paths = 0, n_clusters = 0;
    for (const auto& element : m_elements)
    {
        if (element)
        {
            element->emit(program);
            if (element->is_atomic_element())
            {
                n_paths++;
            }
            else
            {
                n_clusters++;
            }
        }
    }

    auto specifier = select_specifier(m_select_type);
    program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(specifier) });
    program.emplace_back(Instr{ InstrType::CREATE_CLUSTER, reinterpret_cast<void*>(n_paths) });

    if (n_clusters)
    {
        program.emplace_back(Instr{ InstrType::MERGE_CLUSTERS, reinterpret_cast<void*>(n_clusters) });
    }
}

void Query::emit(std::vector<Instr>& program)
{
    std::uint64_t n_paths = 0, n_clusters = 0;
    for (const auto& element : m_elements)
    {
        if (element)
        {
            element->emit(program);
            if (element->is_atomic_element())
            {
                n_paths++;
            }
            else
            {
                n_clusters++;
            }
        }
    }

    auto specifier = select_specifier(m_select_type);
    program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(specifier) });
    program.emplace_back(Instr{ InstrType::CREATE_CLUSTER, reinterpret_cast<void*>(n_paths) });

    if (n_clusters)
    {
        program.emplace_back(Instr{ InstrType::MERGE_CLUSTERS, reinterpret_cast<void*>(n_clusters) });
    }
}

void DisplayOp::emit(std::vector<Instr>& program)
{
    program.emplace_back(Instr{ InstrType::DISPLAY });
}

void DeleteOp::emit(std::vector<Instr>& program)
{
    program.emplace_back(Instr{ InstrType::DELETE });
}

MoveOp::MoveOp(const std::string& path)
{
    m_destination_path = format_path(path);
}

void MoveOp::emit(std::vector<Instr>& program)
{
    program.emplace_back(Instr{ InstrType::MOVE, reinterpret_cast<void*>(&m_destination_path) });
}

CopyOp::CopyOp(const std::string& path)
{
    m_destination_path = format_path(path);
}

void CopyOp::emit(std::vector<Instr>& program)
{
    program.emplace_back(Instr{ InstrType::COPY, reinterpret_cast<void*>(&m_destination_path) });
}

void AST::prune_conflicting_select()
{
    for (auto& query : m_queries)
    {
        if (query)
        {
            bool remaining_children = false;
            for (auto& element : query->m_elements)
            {
                if (element && element->conflicting_select_type(query->m_select_type))
                {
                    element = nullptr;
                }
                else
                {
                    remaining_children = true;
                }
            }

            if (!remaining_children)
            {
                query = nullptr;
            }
        }
    }
}

std::vector<Instr> AST::compile()
{
    std::vector<Instr> program;
    for (int i = m_queries.size() - 1; i >= 0; i--)
    {
        if (m_queries[i])
        {
            m_queries[i]->emit(program);
            m_queries[i]->m_disk_operation->emit(program);
        }
    }
    return program;
}
