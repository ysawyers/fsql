#include "runtime.hpp"

#include <iostream>
#include <format>
#include <thread>

namespace fs = std::filesystem;

std::string unique_filename(const std::string& filename, const fs::path& destination_path)
{
    int duplicates = 0;
    std::string unique_filename = filename;

    while (fs::exists(destination_path / unique_filename))
    {
        unique_filename = filename + std::format(" ({})", ++duplicates);
    }
    return unique_filename;
}

void Cluster::execute(std::function<void(const fs::path& path)> operation)
{
    std::vector<std::thread> threads;

    for (const auto& path : m_paths)
    {
        threads.emplace_back([&]() {
            unpack(path, operation);
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    for (const auto& child : m_children)
    {
        child->execute(operation);
    }
}

void Cluster::unpack(const fs::path& path, std::function<void(const fs::path& path)> operation)
{
    try
    {
        if (m_parent)
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::directory_iterator(path))
                {
                    if (!m_rule || (*m_rule)(nested_path))
                    {
                        m_parent->unpack(nested_path, operation);
                    }
                }
            }
            else
            {
                if (!m_rule || (*m_rule)(path))
                {
                    m_parent->unpack(path, operation);
                }
            }
        }
        else
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::directory_iterator(path))
                {
                    if (!m_rule || (*m_rule)(nested_path))
                    {
                        operation(nested_path);
                    }
                }
            }
            else
            {
                if (!m_rule || (*m_rule)(path))
                {
                    operation(path);
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "could not unpack for: " << path << "\n" << e.what() << "\n";
    }
}

void RecursiveCluster::unpack(const fs::path& path, std::function<void(const fs::path& path)> operation)
{
    try
    {
        if (m_parent)
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::recursive_directory_iterator(path))
                {
                    if (fs::is_regular_file(nested_path) && (!m_rule || (*m_rule)(nested_path)))
                    {
                        m_parent->unpack(nested_path, operation);
                    }
                }
            }
            else
            {
                if (!m_rule || (*m_rule)(path))
                {
                    m_parent->unpack(path, operation);
                }
            }
        }
        else
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::recursive_directory_iterator(path))
                {
                    if (fs::is_regular_file(nested_path) && (!m_rule || (*m_rule)(nested_path)))
                    {
                        operation(nested_path);
                    }
                }
            }
            else
            {
                if (!m_rule || (*m_rule)(path))
                {
                    operation(path);
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "could not unpack recursively for: " << path << "\n" << e.what() << "\n";
    }
}

void DirectoriesCluster::unpack(const fs::path& path, std::function<void(const fs::path& path)> operation)
{
    try
    {
        if (m_parent)
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::directory_iterator(path))
                {
                    if (fs::is_directory(nested_path) && (!m_rule || (*m_rule)(nested_path)))
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
                    if (fs::is_directory(nested_path) && (!m_rule || (*m_rule)(nested_path)))
                    {
                        operation(nested_path);
                    }
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "could not unpack directories for: " << path << "\n" << e.what() << "\n";
    }
    
}

void FilesCluster::unpack(const fs::path& path, std::function<void(const fs::path& path)> operation)
{
    try
    {
        if (m_parent)
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::directory_iterator(path))
                {
                    if (fs::is_regular_file(nested_path) && (!m_rule || (*m_rule)(nested_path)))
                    {
                        m_parent->unpack(nested_path, operation);
                    }
                }
            }
            else
            {
                if (!m_rule || (*m_rule)(path))
                {
                    m_parent->unpack(path, operation);
                }
            }
        }
        else
        {
            if (fs::is_directory(path))
            {
                for (const auto& nested_path : fs::directory_iterator(path))
                {
                    if (fs::is_regular_file(nested_path) && (!m_rule || (*m_rule)(nested_path)))
                    {
                        operation(nested_path);
                    }
                }
            }
            else
            {
                if (!m_rule || (*m_rule)(path))
                {
                    operation(path);
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "could not unpack files for: " << path << "\n" << e.what() << "\n";
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
        std::shared_ptr<Cluster> cluster = nullptr;

        auto select_specifier = reinterpret_cast<std::uint64_t>(stack_pop());
        switch (select_specifier)
        {
        case 0b11:
            cluster = std::make_shared<Cluster>();
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

        cluster->m_rule = reinterpret_cast<std::function<bool(const fs::path&)>*>(stack_pop());

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
    std::unordered_set<fs::path> paths;
    std::mutex paths_mutex;

    auto cluster = m_cluster_stack[--m_cluster_sp];
    cluster->execute([&](const fs::path& path) {
        if (!paths.contains(path))
        {
            printf("%s\n", path.c_str());

            std::lock_guard<std::mutex> guard(paths_mutex);
            paths.insert(path);
        }
    });
}

void Runtime::delete_operation()
{
    auto cluster = m_cluster_stack[--m_cluster_sp];
    cluster->execute([&](const fs::path& path) {
        fs::remove_all(path); // TODO: this seems to not be working for directories with files in it.
    });
}

void Runtime::move_operation(fs::path& destination_path)
{
    std::mutex mutex;

    auto cluster = m_cluster_stack[--m_cluster_sp];
    cluster->execute([&](const fs::path& path) {
        std::lock_guard<std::mutex> guard(mutex);

        fs::rename(path, destination_path / unique_filename(path.filename(), destination_path));
    });
}

void Runtime::copy_operation(fs::path& destination_path)
{
    std::unordered_set<fs::path> paths;
    std::mutex mutex;

    auto cluster = m_cluster_stack[--m_cluster_sp];
    cluster->execute([&](const fs::path& path) {
        if (!paths.contains(path))
        {
            std::lock_guard<std::mutex> guard(mutex);

            fs::copy(path, destination_path / unique_filename(path.filename(), destination_path), fs::copy_options::recursive);
            paths.insert(path);
        }
    });
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
