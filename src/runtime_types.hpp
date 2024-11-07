#ifndef RUNTIME_TYPES_HPP
#define RUNTIME_TYPES_HPP

enum class InstrType
{
    PUSH,
    CREATE_CLUSTER,
    MERGE_CLUSTERS,

    DELETE,
    COPY,
    MOVE,
    DISPLAY
};

struct Instr
{
    InstrType m_type;
    void* m_operand;
};

#endif