#include "builtin_parsers/dot_parser.h"

void dot_parser_t::print_help_message() {
    std::cout << ". <path> [-h|--help]\n";
    std::cout << std::string(2, ' ') << "Execute commands from a file in the current shell.\n\n";
    std::cout << general_opt << std::endl;
}

void dot_parser_t::parse(const std::vector<std::string> &av) {
    // set up parser
    po::command_line_parser parser{av};
    parser.options(general_opt);

    // parse options
    po::parsed_options parsed = parser.run();

    // store options into variable map and notify
    po::store(parsed, var_map);
    po::notify(var_map);

    // analyze options
    if (var_map.count("help")) {
        help_flag = true;
    } else {
        std::vector<std::string> positional_options = po::collect_unrecognized(parsed.options,
                                                                               po::collect_unrecognized_mode::include_positional);
        size_t num_options = positional_options.size();

        if (num_options == 0) {
            throw po::error{"path is not specified"};
        } else if (num_options == 1) {
            path = positional_options[0];
        } else {
            throw po::too_many_positional_options_error();
        }
    }
}
