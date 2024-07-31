#include "parser.hpp"

#include <regex>

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
    const auto& tok = next_token();
    if (tok.m_type == TokenType::STRING) {
        m_program.emplace_back(Instr{InstrType::PUSH_STRING, &tok.m_lexeme});
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
    const auto& tok = next_token();

    if (tok.m_lexeme == "MOVE") {
        if (!has_next_token()) {
            fprintf(stderr, "error: MOVE clause expects preceeding string literal\n");
            return false;
        }

        const auto& tok = next_token();
        if (tok.m_type != TokenType::STRING) {
            fprintf(stderr, "error: Unexpected token, string literal expected after MOVE clause\n");
            return false;
        }
        m_program.emplace_back(Instr{InstrType::MOVE_DE_CONTENTS, &tok.m_lexeme});
    } else if (tok.m_lexeme == "DELETE") {
        m_program.emplace_back(Instr{InstrType::DELETE_DE_CONTENTS});
    } else if (tok.m_lexeme == "COPY") {
        if (!has_next_token()) {
            fprintf(stderr, "error: COPY clause expects preceeding string literal\n");
            return false;
        }

        const auto& tok = next_token();
        if (tok.m_type != TokenType::STRING) {
            fprintf(stderr, "error: Unexpected token, string literal expected after COPY clause\n");
            return false;
        }
        m_program.emplace_back(Instr{InstrType::COPY_DE_CONTENTS, &tok.m_lexeme});
    }

    return true;
}

bool Parser::filtering_clause() {
    const auto& tok = next_token();

    if ((tok.m_lexeme == "INCLUDE") || (tok.m_lexeme == "EXCLUDE")) {
        std::uint64_t is_include_clause = tok.m_lexeme == "INCLUDE";

        if (!has_next_token()) {
            std::cerr << "error: " << tok.m_lexeme << " clause expects preceeding regex string literal\n";
            return false;
        }

        const auto& tok = next_token();
        if (tok.m_type != TokenType::STRING) {
            std::cerr << "error: Unexpected token...\n"; 
            return false;
        }

        try {
            // verifies validity of regex pattern before emitting instructions
            std::regex pattern(tok.m_lexeme);
            m_program.emplace_back(Instr{InstrType::PUSH_STRING, &tok.m_lexeme});
            m_program.emplace_back(Instr{InstrType::CLUSTER_REGEX_MATCH_FILTER, reinterpret_cast<const void*>(is_include_clause)});
        } catch (const std::regex_error&) {
            return false;
        }
    }

    return true;
}

bool Parser::select_clause() {
    std::uint64_t string_elements = 0, disk_elements = 0;

    if (!has_next_token() || (next_token().m_type != TokenType::SELECT_CLAUSE)) {
        fprintf(stderr, "error: Expected SELECT clause\n");
        return false;
    }

    do {
        if (!has_next_token()) {
            // TODO: come up with a better error message
            fprintf(stderr, "error: ????\n");
            return false;
        }

        bool is_disk_element = false;
        if (!virtual_directory(is_disk_element)) return false;

        if (is_disk_element) {
            disk_elements++;
        } else {
            string_elements++;
        }
    } while (has_next_token() && (next_token().m_type == TokenType::COMMA));
    push_back_token();

    m_program.emplace_back(Instr{InstrType::COLLAPSE_TO_CLUSTER, reinterpret_cast<void*>(string_elements)});
    if (disk_elements)
        m_program.emplace_back(Instr{InstrType::COLLAPSE_CLUSTERS, reinterpret_cast<void*>(disk_elements)});

    while (has_next_token()) {
        const auto& tok = next_token();
        push_back_token();

        if (tok.m_type == TokenType::FILTERING_CLAUSE) {
            if (!filtering_clause()) return false;
        } else {
            break;
        }
    }
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

    if (!has_next_token() || (next_token().m_type != TokenType::SEMICOL)) {
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
