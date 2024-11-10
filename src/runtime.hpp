#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <array>
#include <filesystem>
#include <functional>
#include <unordered_set>

#include "runtime_types.hpp"

class Cluster
{
    public:
        Cluster() : m_parent(nullptr) {};

        void execute(std::function<void(const std::filesystem::path& path)> operation);

        virtual void unpack(const std::filesystem::path& path, std::function<void(const std::filesystem::path& path)> operation);

    public:
        std::shared_ptr<Cluster> m_parent;
        std::vector<std::shared_ptr<Cluster>> m_children;
        std::vector<std::filesystem::path> m_paths;
        std::function<bool(const std::filesystem::path&)>* m_rule;
};

class RecursiveCluster : public Cluster
{
    public:
        void unpack(const std::filesystem::path& path, std::function<void(const std::filesystem::path& path)> operation);
};

class DirectoriesCluster : public Cluster
{
    public:
        void unpack(const std::filesystem::path& path, std::function<void(const std::filesystem::path& path)> operation);
};

class FilesCluster : public Cluster
{
    public:
        void unpack(const std::filesystem::path& path, std::function<void(const std::filesystem::path& path)> operation); 
};

class Runtime
{
    public:
        Runtime() : m_operand_sp(0), m_cluster_sp(0) {};

        void run(std::vector<Instr>&& program);

    private:
        void* stack_pop();
        void stack_push(void* operand);

        void create_cluster(std::uint64_t n_paths);
        void merge_clusters(std::uint64_t n_clusters);

        void display_operation();
        void delete_operation();
        void copy_operation(std::filesystem::path& destination_path);
        void move_operation(std::filesystem::path& destination_path);

    private:
        std::array<std::shared_ptr<Cluster>, 1024> m_cluster_stack;
        std::array<void*, 1024> m_operand_stack;
        int m_operand_sp;
        int m_cluster_sp;
};

#endif
