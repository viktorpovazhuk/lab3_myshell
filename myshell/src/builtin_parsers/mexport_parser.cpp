#include "builtin_parsers/mexport_parser.h"

#include <iostream>

void mexport_parser_t::print_help_message() {
    std::cout << "mexport [-h|--help] name1=value1 name2=value2 ...\n";
    std::cout << std::string(2, ' ') << "Create global environment variables.\n\n";
    std::cout << general_opt << std::endl;
}

void mexport_parser_t::configure_parser(po::command_line_parser &parser) {
    builtin_parser_t::configure_parser(parser);
    parser.allow_unregistered();
}

void mexport_parser_t::analyze_parsed_options(po::parsed_options &parsed) {
    assignments = po::collect_unrecognized(parsed.options, po::collect_unrecognized_mode::include_positional);
}
