#ifndef MYSHELL_BUILTIN_PARSER_H
#define MYSHELL_BUILTIN_PARSER_H

#include <boost/program_options.hpp>
#include <string>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>

namespace po = boost::program_options;

class BuiltInOptionsParseException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class builtin_parser_t {
public:
    builtin_parser_t() = default;
    builtin_parser_t(const builtin_parser_t &) = default;
    builtin_parser_t &operator=(const builtin_parser_t &) = delete;
    builtin_parser_t(builtin_parser_t &&) = default;
    builtin_parser_t &operator=(builtin_parser_t &&) = delete;
    ~builtin_parser_t() = default;

    virtual void parse(const std::vector<std::string> &av);

    bool get_help_flag() {
        return help_flag;
    }

    virtual void print_help_message() {
        std::cout << general_opt << std::endl;
    }

    virtual void setup_description();
protected:
    virtual void configure_parser(po::command_line_parser &parser);
    virtual void analyze_parsed_options(po::parsed_options &parsed);
    virtual void analyze_var_map();

    boost::program_options::variables_map var_map{};
    boost::program_options::options_description general_opt{"Allowed options"};
    bool help_flag = false;
};

#endif //MYSHELL_BUILTIN_PARSER_H
