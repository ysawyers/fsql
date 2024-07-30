#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <iostream>
#include <vector>
#include <filesystem>

enum class InstrType {
    PUSH_STRING,
    COLLAPSE_TO_CLUSTER,
    COLLAPSE_CLUSTERS,
    COPY_DE_CONTENTS,
    MOVE_DE_CONTENTS,
    DELETE_DE_CONTENTS,
};

struct Instr {
    InstrType m_type;
    const void* m_operand;
};

class DiskCluster {
    public:

        /*!
            \param[in] path dereferenced as std::string
        */
        void add_element(const void* path);

    public:

        std::vector<std::filesystem::path> m_elements;
};

std::ostream& operator<<(std::ostream& stream, const Instr& instr);
std::ostream& operator<<(std::ostream& stream, const DiskCluster& DiskCluster);

class Runtime {
    public:

        void execute_program(const std::vector<Instr>& program);

    private:

        /*!
            \brief moves each mapped file from the disk cluster and renames 
                   them internally to their new location

            \param[in] path where files will be moved to

            \return true if no critical errors
        */
        bool move_disk_cluster(const void* path);

        /*!
            \brief copies each mapped file from the disk cluster

            \param[in] path where files will be copied to

            \return true if no critical errors
        */
        bool copy_disk_cluster(const void* path);

        /*!
            \brief removes each mapped file from the disk cluster and the filesystem

            \return true if no critical errors
        */
        bool delete_disk_cluster();

        /*!
            \brief attempts to create a directory from an invalid path given by the user to
                   complete an operation that depends on it
            
            \param[in] invalid_dir_path the invalid directory path
            \param[in] disk_cluster     the elements that are involved
            \param[in] operation        the operation that requires a valid directory

            \return true if successfully created directory
        */
        bool create_missing_directory(
            const std::filesystem::path& invalid_dir_path, 
            const DiskCluster& disk_cluster,
            const std::string&& operation
        );

        /*!
            \brief Checks if there will be a conflict when renaming a pathname

            \param[in] original_path    original pathname
            \param[in] renamed_path     new desired pathname

            \return true if there is a conflict
        */
        bool conflicting_pathname(
            const std::filesystem::path& original_path, 
            const std::filesystem::path& renamed_path
        );

        std::vector<const void*> m_operands;
        std::vector<DiskCluster> m_disk_clusters;
};

#endif
