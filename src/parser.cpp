#include "parser.hpp"

const Token& Parser::next_token() {
    return m_tokens[m_curr_token_idx++];
}

void Parser::push_back_token() {
    if (m_curr_token_idx)
        m_curr_token_idx--;
}

bool Parser::has_next_token() {
    return m_curr_token_idx < m_tokens.size();
};

bool Parser::virtual_directory(bool& is_disk_element) {
    if (!has_next_token()) {
        // TODO: better error message here
        fprintf(stderr, "error: ...\n");
        return false;
    }

    const auto& tok = next_token();
    if (tok.m_type == TokenType::STRING) {
        m_program.emplace_back(Instr{ .m_type=InstrType::PUSH_STRING, .m_operand=&tok.m_lexeme });
        is_disk_element = false;
        return true;
    } else if (tok.m_type == TokenType::LPAREN) {
        if (!select_clause()) return false;
        if (!has_next_token() || (next_token().m_type != TokenType::RPAREN)) {
            fprintf(stderr, "error: Expected closing ) for nested sub-query\n");
            return false;
        }
        is_disk_element = true;
        return true;
    }

    fprintf(stderr, "error: Unexpected token, expected string or nested sub-query\n");
    return false;
}

bool Parser::modifying_clause() {
    // only caller is select which guarantees that there is at least one
    // more valid token so has_next_token() check is not required here
    const auto& tok = next_token();

    if (tok.m_lexeme == "MOVE") {
        const auto& tok = next_token();
        if (!has_next_token() || (tok.m_type != TokenType::STRING)) {
            fprintf(stderr, "error: Move clause expects file path\n");
            return false;
        }
        m_program.emplace_back(Instr{ .m_type=InstrType::MOVE_DE_CONTENTS, .m_operand=&tok.m_lexeme });
        return true;
    } else if (tok.m_lexeme == "DELETE") {
        m_program.emplace_back(Instr{ .m_type=InstrType::DELETE_DE_CONTENTS });
        return true;
    } else if (tok.m_lexeme == "COPY") {
        const auto& tok = next_token();
        m_program.emplace_back(Instr{ .m_type=InstrType::COPY_DE_CONTENTS, .m_operand=&tok.m_lexeme });
        return true;
    }

    std::unreachable();
}

bool Parser::select_clause() {
    std::uint64_t string_elements = 0, disk_elements = 0;

    if (!has_next_token() || (next_token().m_type != TokenType::SELECT_CLAUSE)) {
        fprintf(stderr, "error: Expected select clause\n");
        return false;
    }

    do {
        bool is_disk_element = false;
        if (!virtual_directory(is_disk_element)) return false;

        if (is_disk_element) {
            disk_elements++;
        } else {
            string_elements++;
        }
    } while (has_next_token() && (next_token().m_type == TokenType::COMMA));
    push_back_token();

    m_program.emplace_back(Instr{ .m_type=InstrType::COLLAPSE_TO_CLUSTER, .m_operand=reinterpret_cast<void*>(string_elements) });
    if (disk_elements)
        m_program.emplace_back(Instr{ .m_type=InstrType::COLLAPSE_CLUSTERS, .m_operand=reinterpret_cast<void*>(disk_elements) });

    return true;
}

bool Parser::statement() {
    // statement() is invoked under the guarantee there is at least one token
    // so has_next_token() check not required here
    const auto& tok = next_token();
    push_back_token();

    if (tok.m_type == TokenType::SELECT_CLAUSE) {
        if (!select_clause()) return false;

        if (has_next_token()) {
            const auto& tok = next_token();
            push_back_token();

            if (tok.m_type == TokenType::MODIFYING_CLAUSE)
                if (!modifying_clause()) return false;
        }
    } else {
        return false;
    }

    if (!has_next_token()) return false;
    if (next_token().m_type != TokenType::SEMICOL) {
        fprintf(stderr, "error: Expected semi-colon\n");
        return false;
    }
    return true;
}

void Parser::generate_program() {
    bool successful = true;
    while (successful && has_next_token())
        successful = statement();

    if (!successful)
        throw std::runtime_error("failed to execute query!");
}
