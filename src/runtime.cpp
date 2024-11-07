#include "runtime.hpp"

#include <iostream>
#include <format>

namespace fs = std::filesystem;

void Cluster::execute(std::function<void(const std::filesystem::path& path)> operation)
{
    for (const auto& path : m_paths)
    {
        unpack(path, operation);
    }

    // TODO: we might be able to just slap each call to execute in a thread in the future!
    for (const auto& child : m_children)
    {
        child->execute(operation);
    }
}

void RecursiveCluster::unpack(const fs::path& path, std::function<void(const std::filesystem::path& path)> operation)
{
    if (m_parent)
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::recursive_directory_iterator(path))
            {
                if (fs::is_regular_file(nested_path))
                {
                    m_parent->unpack(nested_path, operation);
                }
            }
        }
        else
        {
            m_parent->unpack(path, operation);
        }
    }
    else
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::recursive_directory_iterator(path))
            {
                if (fs::is_regular_file(nested_path))
                {
                    operation(nested_path);
                }
            }
        }
        else
        {
            operation(path);
        }
    }
}

void AllCluster::unpack(const fs::path& path, std::function<void(const std::filesystem::path& path)> operation)
{
    if (m_parent)
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::directory_iterator(path))
            {
                m_parent->unpack(nested_path, operation);
            }
        }
        else
        {
            m_parent->unpack(path, operation);
        }
    }
    else
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::directory_iterator(path))
            {
                operation(nested_path);
            }
        }
        else
        {
            operation(path);
        }
    }
}

void DirectoriesCluster::unpack(const fs::path& path, std::function<void(const std::filesystem::path& path)> operation)
{
    if (m_parent)
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::directory_iterator(path))
            {
                if (fs::is_directory(nested_path))
                {
                    m_parent->unpack(nested_path, operation);
                }
            }
        }
    }
    else
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::directory_iterator(path))
            {
                if (fs::is_directory(nested_path))
                {
                    operation(nested_path);
                }
            }
        }
    }
}

void FilesCluster::unpack(const fs::path& path, std::function<void(const std::filesystem::path& path)> operation)
{
    if (m_parent)
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::directory_iterator(path))
            {
                if (fs::is_regular_file(nested_path))
                {
                    m_parent->unpack(nested_path, operation);
                }
            }
        }
        else
        {
            m_parent->unpack(path, operation);
        }
    }
    else
    {
        if (fs::is_directory(path))
        {
            for (const auto& nested_path : fs::directory_iterator(path))
            {
                if (fs::is_regular_file(nested_path))
                {
                    operation(nested_path);
                }
            }
        }
        else
        {
            operation(path);
        }
    }
}

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

void Runtime::create_cluster(std::uint64_t n_paths)
{
    if (m_cluster_sp < m_cluster_stack.size())
    {
        std::shared_ptr<Cluster> cluster;

        auto select_specifier = reinterpret_cast<std::uint64_t>(stack_pop());
        switch (select_specifier)
        {
        case 0b11:
            cluster = std::make_shared<AllCluster>();
            break;
        case 0b10:
            cluster = std::make_shared<DirectoriesCluster>();
            break;
        case 0b01:
            cluster = std::make_shared<FilesCluster>();
            break;
        case 0b00:
            cluster = std::make_shared<RecursiveCluster>();
            break;
        default: std::unreachable();
        }

        for (; n_paths > 0; n_paths--)
        {
            auto element = *reinterpret_cast<fs::path*>(stack_pop());
            cluster->m_paths.emplace_back(element);
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
    if ((m_cluster_sp - static_cast<std::int64_t>(n_clusters)) >= 0)
    {
        auto parent = m_cluster_stack[--m_cluster_sp];
        for (; n_clusters > 0; n_clusters--)
        {
            auto child = m_cluster_stack[--m_cluster_sp];
            child->m_parent = parent;
            parent->m_children.emplace_back(child);
        }
        m_cluster_stack[m_cluster_sp++] = parent;
    }
    else
    {
        throw std::runtime_error("runtime error: stack underflow");
    }
}

void Runtime::display_operation()
{
    auto cluster = m_cluster_stack[--m_cluster_sp];
    cluster->execute([](const fs::path& path) {
        std::cout << path << "\n";
    });
}

void Runtime::delete_operation()
{
    auto cluster = m_cluster_stack[--m_cluster_sp];
    printf("delete\n");
    exit(1);
}

void Runtime::move_operation(fs::path& destination_path)
{
    auto cluster = m_cluster_stack[--m_cluster_sp];
    printf("move\n");
    exit(1);
}

void Runtime::copy_operation(fs::path& destination_path)
{
    auto cluster = m_cluster_stack[--m_cluster_sp];
    printf("copy\n");
    exit(1);
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
        case InstrType::DISPLAY:
            display_operation();
            break;
        case InstrType::DELETE:
            delete_operation();
            break;
        case InstrType::COPY:
            copy_operation(*reinterpret_cast<fs::path*>(instr.m_operand));
            break;
        case InstrType::MOVE:
            move_operation(*reinterpret_cast<fs::path*>(instr.m_operand));
            break;
        }
    }
}
