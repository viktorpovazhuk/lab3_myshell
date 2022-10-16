//
// Created by Bohdan Shevchuk on 16.10.2022.
//

#ifndef MYSHELL_BUILT_IN_PARSER_H
#define MYSHELL_BUILT_IN_PARSER_H

#include <boost/program_options.hpp>
#include <string>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>

using std::string;

class BuiltInOptionsParseException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class built_in_options_parser_t {
public:
    built_in_options_parser_t() = default;

    //! Explicit is better than implicit:
    built_in_options_parser_t(const built_in_options_parser_t &) = default;

    built_in_options_parser_t &operator=(const built_in_options_parser_t &) = delete;

    built_in_options_parser_t(built_in_options_parser_t &&) = default;

    built_in_options_parser_t &operator=(built_in_options_parser_t &&) = delete;

    ~built_in_options_parser_t() = default;

protected:
    boost::program_options::variables_map var_map{};
    boost::program_options::options_description general_opt{};
    bool help_flag = false;
    std::vector<std::string> args;
    std::unordered_map<string, string> help_map;

};

class com_line_built_in : public built_in_options_parser_t {
public:
    com_line_built_in();

    com_line_built_in(int ac, std::vector<std::string> &av);

    //! Explicit is better than implicit:
    com_line_built_in(const com_line_built_in &) = default;


    com_line_built_in &operator=(const com_line_built_in &) = delete;

    com_line_built_in(com_line_built_in &&) = default;

    com_line_built_in &operator=(com_line_built_in &&) = delete;

    ~com_line_built_in() = default;

    void parse(std::vector<std::string> &av);

    [[nodiscard]] bool get_help_flag() const { return help_flag; };
    [[nodiscard]] std::vector<std::string> get_filenames() const { return args; };
    std::string get_help_msg()  { return help_map[args[0]]; };


};



#endif //MYSHELL_BUILT_IN_PARSER_H
