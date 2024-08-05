#include "runtime.hpp"
#include "common.hpp"

#include <sstream>
#include <regex>

namespace fs = std::filesystem;

std::ostream& operator<<(std::ostream& stream, const Instr& instr) {
    switch (instr.m_type) {
    case InstrType::PUSH:
        stream << "PUSH";
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
    case InstrType::CLUSTER_MODIFIED_DATE_FILTER:
        stream << "CLUSTER_MODIFIED_DATE_FILTER";
        break;
    case InstrType::PRINT_DISK_CLUSTER:
        stream << "PRINT_DISK_CLUSTER";
        break;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const DiskCluster& disk_cluster) {
    for (const auto& element : disk_cluster.m_elements)
        stream << element << "\n";
    return stream;
}

bool DiskCluster::add_element(const fs::path& ambiguous_path, SelectModifier modifier) {
    const auto el = get_absolute_path(ambiguous_path);

    try {
        if (fs::exists(el)) {
            if (fs::is_directory(el)) {
            if (modifier == SelectModifier::RECURSIVE) {
                for (const auto& deeply_nested_el : fs::recursive_directory_iterator{el})
                    m_elements.insert(deeply_nested_el);
            } else {
                for (const auto& nested_el : fs::directory_iterator{el})
                    if (should_include_element(nested_el, modifier)) 
                        m_elements.insert(nested_el);
            }
            } else if (should_include_element(el, modifier)) {
                m_elements.insert(el);
            }
            return true;
        }
        std::cout << "warning: " << el << " does not exist, ignored\n";
    } catch (const fs::filesystem_error& ex) {
        std::cout << "warning: " << ex.what() << "\n";
    }
    return false;
}

fs::path DiskCluster::get_absolute_path(const fs::path& ambiguous_path) {
    if (ambiguous_path.is_absolute()) return ambiguous_path;

    auto path_iter = ambiguous_path.begin();
    if (*path_iter == "~") {
        fs::path expanded_absolute(getenv("HOME"));
        for (auto it = std::next(path_iter, 1); it != ambiguous_path.end(); ++it)
            expanded_absolute /= *it;
        return expanded_absolute;
    }
    return fs::absolute(ambiguous_path);
};

bool DiskCluster::should_include_element(const fs::path& el, const SelectModifier modifier) {
    switch (modifier) {
    case SelectModifier::FILES:
        return !fs::is_directory(el);
    case SelectModifier::DIRECTORIES:
        return fs::is_directory(el);
    case SelectModifier::ANY:
    case SelectModifier::RECURSIVE:
        return true;
    }

    std::unreachable();
}

void Runtime::collapse_to_cluster(const SelectModifier modifier) {
    DiskCluster disk_cluster;
    std::size_t total_elements = reinterpret_cast<const std::size_t>(m_operands.back());
    m_operands.pop_back();

    for (auto i = 0; i < total_elements; i++) {
        fs::path path(*reinterpret_cast<const std::string*>(m_operands.back()));
        disk_cluster.add_element(path, modifier);
        m_operands.pop_back();
    }
    m_disk_clusters.emplace_back(disk_cluster);
}

void Runtime::collapse_clusters(const SelectModifier modifier) {
    std::size_t total_elements = reinterpret_cast<const std::size_t>(m_operands.back()) + 1;
    m_operands.pop_back();

    DiskCluster compressed_cluster;
    for (auto i = 0; i < total_elements; i++) {
        const auto& disk_cluster = m_disk_clusters.back();
        for (const auto& el : disk_cluster.m_elements)
            compressed_cluster.add_element(el, modifier);
        m_disk_clusters.pop_back();
    }
    m_disk_clusters.emplace_back(compressed_cluster);
}

bool Runtime::copy_disk_cluster(const void* path) {
    const fs::path ambiguous_path(*reinterpret_cast<const std::string*>(path));
    const auto dest_dir_path = fs::absolute(ambiguous_path);
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
            std::cout << "warning: " << ex.what() << "\n";
        }
    }

    return true;
}

bool Runtime::move_disk_cluster(const void* path) {
    const fs::path ambiguous_path(*reinterpret_cast<const std::string*>(path));
    const auto dest_dir_path = fs::absolute(ambiguous_path);
    auto& disk_cluster = m_disk_clusters.back();

    if (!fs::exists(dest_dir_path))
        if (!create_missing_directory(dest_dir_path, disk_cluster, "MOVE")) return false;

    if (!fs::is_directory(dest_dir_path)) {
        std::cout << "runtime error: Cannot MOVE files to a non-directory " << dest_dir_path << "\n";
        return false;
    }

    std::set<fs::path> cluster_elements = disk_cluster.m_elements;
    for (const auto& el : cluster_elements) {
        if (el.parent_path() == dest_dir_path) continue;

        const auto renamed_path = dest_dir_path / el.filename();
        if (conflicting_pathname(el, renamed_path)) {
            std::cout << "warning: Cannot MOVE " << el << " to " << dest_dir_path << " due to filename conflict\n";
            continue;
        }

        try {
            fs::rename(el, renamed_path);
            disk_cluster.m_elements.erase(el);
            disk_cluster.m_elements.insert(renamed_path);
        } catch (const fs::filesystem_error& ex) {
            std::cout << "warning: " << ex.what() << "\n";
        }
    }

    return true;
}

bool Runtime::delete_disk_cluster() {
    auto& disk_cluster = m_disk_clusters.back();

    for (auto it = disk_cluster.m_elements.begin(); it != disk_cluster.m_elements.end(); ++it) {
        const auto& el = *it;
        try {
            fs::remove_all(el);
        } catch (const fs::filesystem_error& ex) {
            std::cout << "warning: " << ex.what() << "\n";
        }
    }
    disk_cluster.m_elements.clear();

    return true;
}

bool Runtime::cluster_regex_match_filter(const void* include_or_exclude) {
    const bool include_match = reinterpret_cast<std::uint64_t>(include_or_exclude);
    auto& disk_cluster = m_disk_clusters.back();

    std::regex pattern(*reinterpret_cast<const std::string*>(m_operands.back()));
    m_operands.pop_back();

    std::set<fs::path> matched_elements;
    for (const auto& el : disk_cluster.m_elements) {
        if (include_match == std::regex_match(el.filename().c_str(), pattern))
            matched_elements.insert(el);
    }
    disk_cluster.m_elements = matched_elements;

    return true;
}

bool Runtime::cluster_modified_date_filter(bool select_after) {
    auto& disk_cluster = m_disk_clusters.back();
    const std::string date = *reinterpret_cast<const std::string*>(m_operands.back());
    std::istringstream iss(date);

    std::tm tm{};
    iss >> std::get_time(&tm, "%d/%m/%y-%H:%M:%S");
    if (iss.fail()) {
        std::cout << "runtime error: invalid date string, expected format is DD/MM/YYYY or DD/MM/YYYY-HH:MM:SS\n";
        return false;
    }

    const auto threshold = mktime(&tm);
    std::set<fs::path> matched_elements;
    for (const auto& el : disk_cluster.m_elements) {
        const auto epoch_timestamp = std::chrono::duration_cast<std::chrono::seconds>
            (fs::last_write_time(el).time_since_epoch()).count();

        if (((epoch_timestamp >= threshold) && select_after) || (epoch_timestamp <= threshold))
            matched_elements.insert(el);
    }
    disk_cluster.m_elements = matched_elements;

    return true;
}

bool Runtime::conflicting_pathname(
    const fs::path& original_path, 
    const fs::path& renamed_path
) {
    if (fs::exists(renamed_path)) {
        return !fs::is_directory(original_path) && !fs::is_directory(renamed_path) ||
            fs::is_directory(original_path) && fs::is_directory(renamed_path);
    }
    return false;
}

bool Runtime::create_missing_directory(
    const fs::path& invalid_dir_path, 
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

void Runtime::execute_program() {
    bool fatal_error = false;

    for (const auto& instr : m_program) {
        if (fatal_error) break;

        switch (instr.m_type) {
        case InstrType::PUSH: {
            m_operands.emplace_back(instr.m_operand);
            break;
        }
        case InstrType::COLLAPSE_TO_CLUSTER: {
            const SelectModifier modifier = 
                static_cast<const SelectModifier>(
                    reinterpret_cast<const std::uint64_t>(instr.m_operand));
            collapse_to_cluster(modifier);
            break;
        }
        case InstrType::COLLAPSE_CLUSTERS: {
            const SelectModifier modifier = 
                static_cast<const SelectModifier>(
                    reinterpret_cast<const std::uint64_t>(instr.m_operand));
            collapse_clusters(modifier);
            break;
        }
        case InstrType::CLUSTER_REGEX_MATCH_FILTER: {
            fatal_error = !cluster_regex_match_filter(instr.m_operand);
            break;
        }
        case InstrType::CLUSTER_MODIFIED_DATE_FILTER: {
            fatal_error = !cluster_modified_date_filter(instr.m_operand);
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
        case InstrType::PRINT_DISK_CLUSTER: {
            std::cout << m_disk_clusters.back() << std::endl;
            break;
        }
        }
    }
}
