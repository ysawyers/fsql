#include "runtime.hpp"

#include <regex>

#include "common.hpp"

namespace fs = std::filesystem;

std::ostream& operator<<(std::ostream& stream, const Instr& instr) {
    switch (instr.m_type) {
    case InstrType::PUSH_STRING:
        stream << "PUSH_STRING";
        break;
    case InstrType::COLLAPSE_TO_CLUSTER:
        stream << "COLLAPSE_TO_CLUSTER";
        break;
    case InstrType::COLLAPSE_CLUSTERS:
        stream << "COLLAPSE_CLUSTERS";
        break;
    case InstrType::MOVE_DE_CONTENTS:
        stream << "MOVE_DE_CONTENTS";
        break;
    case InstrType::COPY_DE_CONTENTS:
        stream << "COPY_DE_CONTENTS";
        break;
    case InstrType::DELETE_DE_CONTENTS:
        stream << "DELETE_DE_CONTENTS";
        break;
    case InstrType::CLUSTER_REGEX_MATCH_FILTER:
        stream << "CLUSTER_REGEX_MATCH_FILTER";
        break;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const DiskCluster& disk_cluster) {
    for (const auto& element : disk_cluster.m_elements)
        stream << element << "\n";
    return stream;
}

void DiskCluster::add_element(const void* path) {
    fs::path ambiguous_path(*reinterpret_cast<const std::string*>(path));
    auto absolute_path = fs::absolute(ambiguous_path);

    if (fs::exists(absolute_path)) {
        if (fs::is_directory(absolute_path)) {
            for (auto const& file : fs::directory_iterator{absolute_path})
                m_elements.emplace_back(file);
        } else {
            m_elements.emplace_back(std::move(absolute_path));
        }
    } else {
        std::cout << "warning: " << absolute_path << " does not exist, ignored\n";
    }
}

bool Runtime::copy_disk_cluster(const void* path) {
    fs::path ambiguous_path(*reinterpret_cast<const std::string*>(path));
    auto dest_dir_path = fs::absolute(ambiguous_path);
    const auto& disk_cluster = m_disk_clusters.back();

    if (!fs::exists(dest_dir_path))
        if (!create_missing_directory(dest_dir_path, disk_cluster, "COPY")) return false;

    if (!fs::is_directory(dest_dir_path)) {
        std::cout << "runtime error: Cannot COPY files to a non-directory " << dest_dir_path << "\n";
        return false;
    }

    for (const auto& el : disk_cluster.m_elements) {
        if (el.parent_path() == dest_dir_path) continue;

        const auto renamed_path = dest_dir_path / el.filename();
        if (conflicting_pathname(el, renamed_path)) {
            std::cout << "warning: Cannot COPY " << el << " to " << dest_dir_path << " due to filename conflict\n";
            continue;
        }

        try {
            if (fs::is_directory(el)) {
                // std::filesystem::copy() on directories will copy over the contents of the folder 
                // but not the folder itself so this handles that corner case

                const auto parent_dir = dest_dir_path / el.filename();
                fs::create_directory(parent_dir);
                fs::copy(el, parent_dir, fs::copy_options::recursive);
            } else {
                fs::copy(el, dest_dir_path);
            }
        } catch (const fs::filesystem_error& ex) {
            std::cout << ex.what() << "\n";
            std::cout << "warning: Failed to COPY " << el << " to " <<  dest_dir_path << "\n";
        } catch (...) {
            std::cout << "warning: Unexpected failure copying " << el << " to " <<  dest_dir_path << "\n";
        }
    }

    return true;
}

bool Runtime::move_disk_cluster(const void* path) {
    fs::path ambiguous_path(*reinterpret_cast<const std::string*>(path));
    auto dest_dir_path = fs::absolute(ambiguous_path);
    auto& disk_cluster = m_disk_clusters.back();

    if (!fs::exists(dest_dir_path))
        if (!create_missing_directory(dest_dir_path, disk_cluster, "MOVE")) return false;

    if (!fs::is_directory(dest_dir_path)) {
        std::cout << "runtime error: Cannot MOVE files to a non-directory " << dest_dir_path << "\n";
        return false;
    }

    for (auto& el : disk_cluster.m_elements) {
        if (el.parent_path() == dest_dir_path) continue;

        const auto renamed_path = dest_dir_path / el.filename();
        if (conflicting_pathname(el, renamed_path)) {
            std::cout << "warning: Cannot MOVE " << el << " to " << dest_dir_path << " due to filename conflict\n";
            continue;
        }

        try {
            fs::rename(el, renamed_path);
            el = renamed_path;
        } catch (const fs::filesystem_error& ex) {
            std::cout << ex.what() << "\n";
            std::cout << "warning: Failed to MOVE " << el << " to " <<  dest_dir_path << "\n";
        } catch (...) {
            std::cout << "warning: Unexpected failure moving " << el << " to " <<  dest_dir_path << "\n";
        }
    }

    return true;
}

bool Runtime::delete_disk_cluster() {
    auto& disk_cluster = m_disk_clusters.back();

    while (disk_cluster.m_elements.size()) {
        const auto& el = disk_cluster.m_elements.back();
        try {
            fs::remove_all(el);
        } catch (const fs::filesystem_error& ex) {
            std::cout << ex.what() << "\n";
            std::cout << "warning: Failed to delete " << el << "\n";
        } catch (...) {
            std::cout << "warning: Unexpected failure deleting " << el << "\n";
        }
        disk_cluster.m_elements.pop_back();
    }

    return true;
}

bool Runtime::cluster_regex_match_filter(const void* include_or_exclude) {
    const bool include_match = reinterpret_cast<std::uint64_t>(include_or_exclude);
    auto& disk_cluster = m_disk_clusters.back();

    std::regex pattern(*reinterpret_cast<const std::string*>(m_operands.back()));
    m_operands.pop_back();

    std::vector<fs::path> matched_elements;
    for (const auto& el : disk_cluster.m_elements) {
        if (include_match == std::regex_match(el.filename().c_str(), pattern))
            matched_elements.emplace_back(el);
    }
    disk_cluster.m_elements = matched_elements;

    return true;
}

bool Runtime::conflicting_pathname(
    const std::filesystem::path& original_path, 
    const std::filesystem::path& renamed_path
) {
    if (fs::exists(renamed_path)) {
        return !fs::is_directory(original_path) && !fs::is_directory(renamed_path) ||
            fs::is_directory(original_path) && fs::is_directory(renamed_path);
    }
    return false;
}

bool Runtime::create_missing_directory(
    const std::filesystem::path& invalid_dir_path, 
    const DiskCluster& disk_cluster,
    const std::string&& operation
) {
    std::cout << "warning: Attempted to " << operation << " the following to " 
        << invalid_dir_path
        << " which does not exist\n\n"
        << disk_cluster
        << "\ndo you want to create this directory? (Y/n) ";

    std::string should_create = "";
    std::getline(std::cin, should_create);
    common::tolower(should_create);

    if (should_create.empty() || should_create == "y" || should_create == "yes") {
        if (!fs::create_directory(invalid_dir_path)) {
            std::cout << "runtime error: Failed to create " << invalid_dir_path << " as a directory\n";
            return false;
        }
        return true;
    }
    std::cout << "runtime error: Refused to create required directory\n";
    return false;
}

void Runtime::execute_program(const std::vector<Instr>& program) {
    bool fatal_error = false;

    for (const auto& instr : program) {
        if (fatal_error) break;

        switch (instr.m_type) {
        case InstrType::PUSH_STRING: {
            m_operands.emplace_back(instr.m_operand);
            break;
        }
        case InstrType::COLLAPSE_TO_CLUSTER: {
            DiskCluster disk_element;
            for (int i = 0; i < reinterpret_cast<const std::size_t>(instr.m_operand); i++) {
                disk_element.add_element(m_operands.back());
                m_operands.pop_back();
            }
            m_disk_clusters.emplace_back(std::move(disk_element));
            break;
        }
        case InstrType::COLLAPSE_CLUSTERS: {
            auto& first = m_disk_clusters[m_disk_clusters.size() - 2].m_elements;
            auto& second = m_disk_clusters[m_disk_clusters.size() - 1].m_elements;
            std::move(second.begin(), second.end(), std::back_inserter(first));
            m_disk_clusters.pop_back();
            break;
        }
        case InstrType::CLUSTER_REGEX_MATCH_FILTER: {
            fatal_error = !cluster_regex_match_filter(instr.m_operand);
            break;
        }
        case InstrType::COPY_DE_CONTENTS: {
            fatal_error = !copy_disk_cluster(instr.m_operand);
            break;
        }
        case InstrType::MOVE_DE_CONTENTS: {
            fatal_error = !move_disk_cluster(instr.m_operand);
            break;
        }
        case InstrType::DELETE_DE_CONTENTS: {
            fatal_error = !delete_disk_cluster();
            break;
        }
        }
    }
}
