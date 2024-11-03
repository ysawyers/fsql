#include "runtime.hpp"

#include <iostream>

namespace fs = std::filesystem;

void* Runtime::stack_pop()
{
    if (m_operand_sp > 0)
    {
        return m_operand_stack[--m_operand_sp];
    }
    throw std::runtime_error("runtime error: stack underflow");
}

void Runtime::stack_push(void* operand)
{
    if (m_operand_sp < m_operand_stack.size())
    {
        m_operand_stack[m_operand_sp++] = operand;
    }
    else
    {
        throw std::runtime_error("runtime error: stack overflow");
    }
}

void Runtime::unpack_all(std::shared_ptr<std::set<fs::path>> cluster, fs::path& directory)
{
    for (const auto& path : fs::directory_iterator(directory))
    {
        cluster->insert(path);
    }
}

void Runtime::unpack_files(std::shared_ptr<std::set<fs::path>> cluster, fs::path& directory)
{
    for (const auto& path : fs::directory_iterator(directory))
    {
        if (fs::is_regular_file(path))
        {
            cluster->insert(path);
        }
    }
}

void Runtime::unpack_directories(std::shared_ptr<std::set<fs::path>> cluster, fs::path& directory)
{
    for (const auto& path : fs::directory_iterator(directory))
    {
        if (fs::is_directory(path))
        {
            cluster->insert(path);
        }
    }
}

void Runtime::create_cluster(std::uint64_t n_paths)
{
    auto select_specifier = reinterpret_cast<std::uint64_t>(stack_pop());

    if (m_cluster_sp < m_cluster_stack.size())
    {
        auto cluster = std::make_shared<std::set<fs::path>>();
        for (; n_paths > 0; n_paths--)
        {
            auto element = *reinterpret_cast<fs::path*>(stack_pop());
            if (fs::is_directory(element))
            {
                // TODO: proper error handling here
                switch (select_specifier)
                {
                case 3:
                    unpack_all(cluster, element);
                    break;
                case 2:
                    unpack_files(cluster, element);
                    break;
                case 1:
                    unpack_directories(cluster, element);
                    break;
                }
            }
            else
            {
                cluster->insert(element);
            }
        }
        m_cluster_stack[m_cluster_sp++] = cluster;
    }
    else
    {
        throw std::runtime_error("runtime error: stack overflow");
    }
}

void Runtime::merge_clusters(std::uint64_t n_clusters)
{
    auto select_specifier = reinterpret_cast<std::uint64_t>(stack_pop());

    if ((m_cluster_sp - static_cast<std::int64_t>(n_clusters)) >= 0)
    {
        auto merged_cluster = std::make_shared<std::set<fs::path>>();
        for (; n_clusters > 0; n_clusters--)
        {
            auto& cluster = m_cluster_stack[--m_cluster_sp];
            merged_cluster->merge(*cluster);
            cluster = nullptr;
        }
        m_cluster_stack[m_cluster_sp++] = merged_cluster;
    }
    else
    {
        throw std::runtime_error("runtime error: stack underflow");
    }
}

void Runtime::run(std::vector<Instr>&& program)
{
    for (const auto& instr : program)
    {
        switch (instr.m_type)
        {
        case InstrType::PUSH:
            stack_push(instr.m_operand);
            break;
        case InstrType::CREATE_CLUSTER:
            create_cluster(reinterpret_cast<std::uint64_t>(instr.m_operand));
            break;
        case InstrType::MERGE_CLUSTERS:
            merge_clusters(reinterpret_cast<std::uint64_t>(instr.m_operand));
            break;
        }
    }

    // NOTE: just for debugging purposes
    if ((m_cluster_sp >= 0) && m_cluster_stack[m_cluster_sp - 1])
    {
        for (const auto& element : *m_cluster_stack[m_cluster_sp - 1])
        {
            std::cout << element << std::endl;
        }
    }
}
