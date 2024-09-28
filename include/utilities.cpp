//
// Created by itsmateusz on 25.09.24.
//

#include "./utilities.hpp"

struct std::tm *current_time() {
    std::time_t raw_time = std::time(nullptr);
    return std::localtime(&raw_time);
}

void convert_str_to_array(const std::string &str, char *destination) {
    for (size_t k = 0; k < (str.length() + 1); ++k) {
        destination[k] = str[k];
    }
}
