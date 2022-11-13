#include "builtin_parsers/mecho_parser.h"

#include <iostream>
#include <boost/program_options.hpp>

void mecho_parser_t::print_help_message() {
    std::cout << "mecho [-h|--help] TEXT1 TEXT2 ...\n";
    std::cout << std::string(2, ' ') << "Print the text.\n\n";
    std::cout << general_opt << std::endl;
}

void mecho_parser_t::parse(const std::vector<std::string> &av) {
    po::command_line_parser parser{av};
    parser.options(general_opt).allow_unregistered();

    po::parsed_options parsed = parser.run();

    po::store(parsed, var_map);
    po::notify(var_map);

    if (var_map.count("help")) {
        help_flag = true;
    } else {
        texts = po::collect_unrecognized(parsed.options,
                                         po::collect_unrecognized_mode::include_positional);
    }
}
