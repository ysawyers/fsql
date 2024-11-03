#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <array>
#include <filesystem>
#include <vector>
#include <set>

#include "runtime_types.hpp"

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

        void unpack_all(std::shared_ptr<std::set<std::filesystem::path>> cluster, std::filesystem::path& directory);
        void unpack_files(std::shared_ptr<std::set<std::filesystem::path>> cluster, std::filesystem::path& directory);
        void unpack_directories(std::shared_ptr<std::set<std::filesystem::path>> cluster, std::filesystem::path& directory);

    private:
        std::array<std::shared_ptr<std::set<std::filesystem::path>>, 1024> m_cluster_stack;
        std::array<void*, 1024> m_operand_stack;
        int m_operand_sp;
        int m_cluster_sp;
};

#endif
