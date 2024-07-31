#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <iostream>
#include <vector>
#include <filesystem>
#include <set>

enum class InstrType {
    // general instructions
    PUSH_STRING,
    COLLAPSE_TO_CLUSTER,
    COLLAPSE_CLUSTERS,
    SET_COLLAPSE_MODIFIER,

    CLUSTER_REGEX_MATCH_FILTER,

    // filesystem operations
    COPY_DE_CONTENTS,
    MOVE_DE_CONTENTS,
    DELETE_DE_CONTENTS,
};

struct Instr {
    InstrType m_type;
    const void* m_operand;
};

struct DiskCluster {
    std::set<std::filesystem::path> m_elements;
};

std::ostream& operator<<(std::ostream& stream, const Instr& instr);
std::ostream& operator<<(std::ostream& stream, const DiskCluster& DiskCluster);

class Runtime {
    public:

        void execute_program(const std::vector<Instr>& program);

    private:

        /*!
            \brief Creates a new disk cluster on the stack

            \param[in] n number of strings (filepaths) on the operand stack that should 
                         be collapsed into a disk cluster
        */
        void collapse_to_cluster(const std::size_t n);

        /*!
            \brief Compresses disk clusters into a single disk cluster on the stack

            \param[in] n number of disk clusters on the stack that should be
                         collapsed (condensed) into a single one
        */
        void collapse_clusters(const std::size_t n);

        /*!
            \brief Filters out elements from a cluster based on a regex pattern

            \param[in] include_or_exclude if set, only keep filenames in the disk cluster that match the regex
                                          otherwise exclude the filenames that match the regex

            \return true if no critical errors
        */
        bool cluster_regex_match_filter(const void* include_or_exclude);

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

        /*!
            \brief Determines if an element should be added to a cluster based on
                   the modifier set at the moment during runtime

            \return true if the element should be added
        */
        bool should_include_element(const std::filesystem::path& element);

        //! 0 = files only, 1 = directories only, -1 = all
        int m_collapse_modifier = -1;

        std::vector<const void*> m_operands;
        std::vector<DiskCluster> m_disk_clusters;
};

#endif
