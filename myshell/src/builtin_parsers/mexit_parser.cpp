#include "builtin_parsers/mexit_parser.h"

#include <iostream>

void mexit_parser_t::print_help_message() {
    std::cout << "mexit [-h,--help] [[-c|--code] CODE]\n";
    std::cout << std::string(2, ' ') << "Exit the shell.\n\n";
    std::cout << general_opt << std::endl;
}

void mexit_parser_t::setup_description() {
    builtin_parser_t::setup_description();
    general_opt.add_options()
            ("code,c", po::value<int>()->default_value(0)->notifier(
                    [](int value)
                    {
                        if (value < 0 || value > 255) {
                            throw po::error("Passed exit code is out of range [0, 255]");
                        }
                    }), "Error code, must satisfy 0 <= code <= 255");
    pos_options.add("code", 1);
}

void mexit_parser_t::parse(const std::vector<std::string> &av) {
    // set up parser
    po::command_line_parser parser{av};
    parser.options(general_opt);
    parser.positional(pos_options);

    // parse options
    po::parsed_options parsed = parser.run();

    // store options into variable map and notify
    po::store(parsed, var_map);
    po::notify(var_map);

    // analyze options
    if (var_map.count("help")) {
        help_flag = true;
    } else {
        code = var_map["code"].as<int>();
    }
}