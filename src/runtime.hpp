#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <iostream>
#include <vector>
#include <filesystem>

enum class InstrType {
    PUSH_STRING,
    COLLAPSE_STR_TO_DE,
    COLLAPSE_DE,
};

struct Instr {
    InstrType m_type;
    const void* m_operand;
};

class DiskElement {
    public:

        /*!
            \param[in] element dereferenced as std::string
        */
        void add_element(const void* element);

    public:

        std::vector<std::filesystem::path> m_files;
};

std::ostream& operator<<(std::ostream& stream, const Instr& instr);
std::ostream& operator<<(std::ostream& stream, const DiskElement& diskElement);

class Runtime {
    public:

        void execute_program(const std::vector<Instr>& program);

    private:

        std::vector<const void*> m_operands;
        std::vector<DiskElement> m_disk_els;
};

#endif

// std::vector<std::filesystem::path>