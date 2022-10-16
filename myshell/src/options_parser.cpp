// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "options_parser.h"
#include <vector>
#include <string>
#include <utility>

namespace po = boost::program_options;

using std::string;

command_line_options_t::command_line_options_t() {
    general_opt.add_options()
            ("help,h,hl",
             "Show help message")
            ("script,s",
             po::value<std::string>(),
             "Script for execution");

    positional_opt.add("script", 1);
}

command_line_options_t::command_line_options_t(int ac, char **av) :
        command_line_options_t() // Delegate constructor
{
    parse(ac, av);
}

void command_line_options_t::parse(int ac, char **av) {
    try {
        po::parsed_options parsed = po::command_line_parser(ac, av)
                .options(general_opt)
                .positional(positional_opt)
                .run();
        po::store(parsed, var_map);
        notify(var_map);

        if (var_map.count("help")) {
            std::cout << "varmap " <<  var_map.count("help") << std:: endl;
            std::cout << general_opt << "\n";
            exit(EXIT_SUCCESS);
        }

        if (var_map.count("script") > 0) {
            script_path = var_map["script"].as<std::string>();
        }
    } catch (std::exception &ex) {
        throw OptionsParseException(ex.what()); // Convert to our error type
    }
}
