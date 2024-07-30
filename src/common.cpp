#include "common.hpp"

#include <algorithm>
#include <cctype>

namespace common {
    void tolower(std::string& value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
    }
}
