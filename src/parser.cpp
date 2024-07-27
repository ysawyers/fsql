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
    // assumes caller performs the bounds check so this is safe
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

bool Parser::action_clause() {
    return false;
}

bool Parser::modifying_clause() {
    return false;
}

bool Parser::select_clause() {
    std::uint64_t string_elements = 0, disk_elements = 0;

    if (!has_next_token() || next_token().m_lexeme != "SELECT") {
        fprintf(stderr, "error: Expected select clause\n");
        return false;
    }

    do {
        if (!has_next_token()) {
            fprintf(stderr, "error: Expected string or nested sub-query near SELECT clause\n");
            return false;
        }

        bool is_disk_element = false;
        if (!virtual_directory(is_disk_element)) return false;

        if (is_disk_element) {
            disk_elements++;
        } else {
            string_elements++;
        }
    } while (has_next_token() && next_token().m_type == TokenType::COMMA);
    push_back_token();

    m_program.emplace_back(Instr{ .m_type=InstrType::COLLAPSE_STR_TO_DE, .m_operand=reinterpret_cast<void*>(string_elements) });
    if (disk_elements)
        m_program.emplace_back(Instr{ .m_type=InstrType::COLLAPSE_DE, .m_operand=reinterpret_cast<void*>(disk_elements) });

    return true;
}

bool Parser::clause() {
    const auto& clause = next_token().m_lexeme; 

    if (clause == "SELECT") {
        push_back_token();
        return select_clause();
    }

    fprintf(stderr, "error: expected clause\n");
    return false;
}

bool Parser::statement() {
    // a statement is guaranteed to contain at least one token so it's safe
    // to call clause() here directly (guarantee provided by caller)
    if (!clause()) return false;

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
