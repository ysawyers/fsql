#include "runtime.hpp"

namespace fs = std::filesystem;

std::ostream& operator<<(std::ostream& stream, const Instr& instr) {
    switch (instr.m_type) {
    case InstrType::PUSH_STRING:
        stream << "PUSH_STRING";
        break;
    case InstrType::COLLAPSE_STR_TO_DE:
        stream << "COLLAPSE_STR_TO_DE";
        break;
    case InstrType::COLLAPSE_DE:
        stream << "COLLAPSE_DE";
        break;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const DiskElement& diskElement) {
    for (int i = 0; i < diskElement.m_files.size(); i++)
        stream << diskElement.m_files[i] << "\n";
    return stream;
}

void DiskElement::add_element(const void* element) {
    fs::path ambiguous_path(*reinterpret_cast<const std::string*>(element));
    auto absolute_path = fs::absolute(ambiguous_path);

    if (fs::exists(absolute_path)) {
        if (fs::is_directory(absolute_path)) {
            for (auto const& dir_file : fs::directory_iterator{absolute_path}) {
                m_files.emplace_back(dir_file);
            }
        } else {
            m_files.emplace_back(std::move(absolute_path));
        }
    } else {
        std::cout << "warning: " << absolute_path << " does not exist\n";
    }
}

void Runtime::execute_program(const std::vector<Instr>& program) {
    for (const auto& instr : program) {
        switch (instr.m_type) {
        case InstrType::PUSH_STRING: {
            m_operands.emplace_back(instr.m_operand);
            break;
        }
        case InstrType::COLLAPSE_STR_TO_DE: {
            DiskElement disk_element;
            for (int i = 0; i < reinterpret_cast<const std::size_t>(instr.m_operand); i++) {
                disk_element.add_element(m_operands.back());
                m_operands.pop_back();
            }
            m_disk_els.emplace_back(std::move(disk_element));
            break;
        }
        case InstrType::COLLAPSE_DE: {
            auto& first = m_disk_els[m_disk_els.size() - 2].m_files;
            auto& second = m_disk_els[m_disk_els.size() - 1].m_files;
            std::move(second.begin(), second.end(), std::back_inserter(first));
            m_disk_els.pop_back();
            break;
        }
        }
    }

    for (int i = 0; i < m_disk_els.size(); i++) {
        std::cout << m_disk_els[i];
    }
}
