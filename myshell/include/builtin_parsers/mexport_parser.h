#ifndef MYSHELL_MEXPORT_PARSER_H
#define MYSHELL_MEXPORT_PARSER_H

#include <boost/program_options.hpp>

#include "builtin_parser.h"

namespace po = boost::program_options;

class mexport_parser_t : public builtin_parser_t {
public:
    mexport_parser_t() = default;
    mexport_parser_t(const mexport_parser_t &) = default;
    mexport_parser_t &operator=(const mexport_parser_t &) = delete;
    mexport_parser_t(mexport_parser_t &&) = default;
    mexport_parser_t &operator=(mexport_parser_t &&) = delete;
    ~mexport_parser_t() = default;

    const std::vector<std::string>& get_assignments() { return assignments; }
    virtual void print_help_message() override;

protected:
    virtual void configure_parser(po::command_line_parser &parser) override;
    virtual void analyze_parsed_options(po::parsed_options &parsed) override;

    std::vector<std::string> assignments;
};


#endif //MYSHELL_MEXPORT_PARSER_H
