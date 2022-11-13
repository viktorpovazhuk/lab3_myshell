#include <boost/program_options.hpp>

#include "builtin_parsers/merrno_parser.h"

namespace po = boost::program_options;

void merrno_parser_t::print_help_message() {
    std::cout << "merrno [-h|--help]\n";
    std::cout << std::string(2, ' ') << "Return exit code of the last command.\n\n";
    std::cout << general_opt << std::endl;
}
