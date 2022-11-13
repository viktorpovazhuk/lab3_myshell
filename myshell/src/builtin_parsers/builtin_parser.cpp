// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "builtin_parsers/builtin_parser.h"
#include <vector>
#include <string>
#include <utility>

namespace po = boost::program_options;

//com_line_built_in::com_line_built_in() {
//    setup_description();
//
//    help_map["merrno"] = "";
//    help_map["mpwd"] = "Returns current working directory. Does not rake params.";
//    help_map["mexport"] = "Allows exporting variables to current environment. Takes any number of variables.\nUsage:\nmexport [variable=value] [variable2=value2]";
//    general_opt.add_options()
//            ("help,hl,h",
//             "Show help message");
//    help_map["mexit"] = "Exits myshell with 0 exit code or other, if given as parameter.";
//    help_map["mecho"] = "Prints text passed as parameters to stdout. Supports environmental variables.";
//    help_map["mcd"] = "Changes current working directory. Supports . .. and ~";
//
//}

void builtin_parser_t::parse(const std::vector<std::string> &av) {
    // set up parser
    po::command_line_parser parser{av};
    configure_parser(parser);

    // parse options
    po::parsed_options parsed = parser.run();
    analyze_parsed_options(parsed);

    // store options into variable map and notify
    po::store(parsed, var_map);
    po::notify(var_map);

    // analyze options
    analyze_var_map();
}

void builtin_parser_t::configure_parser(po::command_line_parser &parser) {
    parser.options(general_opt);
}

void builtin_parser_t::analyze_parsed_options(po::parsed_options &parsed) {

}

void builtin_parser_t::analyze_var_map() {
    if (var_map.count("help")) {
        help_flag = true;
    }
}

void builtin_parser_t::setup_description() {
    general_opt.add_options()
            ("help,h",
             "Show this help message");
}
