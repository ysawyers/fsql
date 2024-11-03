#include "ast.hpp"

#include <iostream>

namespace fs = std::filesystem;

AtomicElement::AtomicElement(std::string& path)
{
    if (!path.size())
    {
        throw std::runtime_error("compilation error: cannot specify an empty path");
    }

    if (path.substr(0, 2) == "~/")
    {
        m_path = fs::path(getenv("HOME")) / fs::path(path.substr(2));
    }
    else if (path.at(0) == '.')
    {
        m_path = fs::current_path();
    }
    else
    {
        m_path = std::filesystem::absolute(path);
    }
}

bool AtomicElement::contains_invalid_paths()
{
    try
    {
        return !fs::exists(m_path);
    }
    catch(const std::exception& e)
    {
        std::cerr << "compilation error: unexpected error while validating " << m_path << "\n"
            << e.what() << "\n";
        throw std::runtime_error("");
    }
}

bool AtomicElement::conflicting_select_type(lexer::TokenType parent_select_type)
{
    return (parent_select_type == lexer::TokenType::DIRECTORIES) && fs::is_regular_file(m_path);
}

void AtomicElement::emit(std::vector<Instr>& program)
{
    program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(&m_path) });
}

bool CompoundElement::contains_invalid_paths()
{
    bool remaining_children = false;
    for (auto& element : m_elements)
    {
        if (element && element->contains_invalid_paths())
        {
            element = nullptr;
        }
        else
        {
            remaining_children = true;
        }
    }
    return !remaining_children;
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
    
    if (!remaining_children)
    {
        return true;
    }

    switch (parent_select_type)
    {
    case lexer::TokenType::ALL: return false;
    case lexer::TokenType::FILES: return m_select_type == lexer::TokenType::DIRECTORIES;
    case lexer::TokenType::DIRECTORIES: return m_select_type == lexer::TokenType::FILES;
    default: std::unreachable();
    }
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

    std::uint64_t select_specifier = m_select_type == lexer::TokenType::ALL ? 3
        : m_select_type == lexer::TokenType::FILES ? 2 : 1;

    if (n_paths)
    {
        program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(select_specifier) });
        program.emplace_back(Instr{ InstrType::CREATE_CLUSTER, reinterpret_cast<void*>(n_paths) });
        n_clusters++;
    }
    if (n_clusters >= 2)
    {
        program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(select_specifier) });
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

    std::uint64_t select_specifier = m_select_type == lexer::TokenType::ALL ? 3
        : m_select_type == lexer::TokenType::FILES ? 2 : 1;

    if (n_paths)
    {
        program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(select_specifier) });
        program.emplace_back(Instr{ InstrType::CREATE_CLUSTER, reinterpret_cast<void*>(n_paths) });
        n_clusters++;
    }
    if (n_clusters >= 2)
    {
        program.emplace_back(Instr{ InstrType::PUSH, reinterpret_cast<void*>(select_specifier) });
        program.emplace_back(Instr{ InstrType::MERGE_CLUSTERS, reinterpret_cast<void*>(n_clusters) });
    }
}

void AST::path_validation()
{
    for (auto& query : m_queries)
    {
        if (query)
        {
            bool remaining_children = false;
            for (auto& element : query->m_elements)
            {
                if (element && element->contains_invalid_paths())
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
        }
    }
    return program;
}
