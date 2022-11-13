#include "builtin_parsers/mpwd_parser.h"

void mpwd_parser_t::print_help_message() {
    std::cout << "mpwd [-h|--help]\n";
    std::cout << std::string(2, ' ') << "Print the name of the current working directory.\n\n";
    std::cout << general_opt << std::endl;
}

