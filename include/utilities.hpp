//
// Created by itsmateusz on 22.09.24.
//

#ifndef TRAINING_UTILITIES_HPP
#define TRAINING_UTILITIES_HPP

#include <ctime>
#include <string>

struct std::tm *current_time();

void convert_str_to_array(const std::string &, char *);

#endif //TRAINING_UTILITIES_HPP
