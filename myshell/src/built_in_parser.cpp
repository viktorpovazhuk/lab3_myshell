// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "built_in_parser.h"
#include <vector>
#include <string>
#include <utility>

namespace po = boost::program_options;

using std::string;

com_line_built_in::com_line_built_in() {
    general_opt.add_options()
            ("help,hl,h",
             "Show help message");

    help_map["merrno"] = "Built-in command that returns error code for the last executed command. Does not take params.";
    help_map["mpwd"] = "Returns current working directory. Does not rake params.";
    help_map["mexport"] = "Allows exporting variables to current environment. Takes any number of variables.\nUsage:\nmexport [variable=value] [variable2=value2]";
    help_map["mexit"] = "Exits myshell with 0 exit code or other, if given as parameter.";
    help_map["mecho"] = "Prints text passed as parameters to stdout. Supports environmental variables.";
    help_map["mcd"] = "Changes current working directory. Supports . .. and ~";

}

com_line_built_in::com_line_built_in(int ac, std::vector<std::string> &av) :
        com_line_built_in() // Delegate constructor
{
    parse(av);
}

void com_line_built_in::parse(std::vector<std::string> &av) {
    try {
        po::parsed_options parsed = po::command_line_parser(av)
                .options(general_opt)
                .allow_unregistered()
                .run();
        args = po::collect_unrecognized(parsed.options, po::include_positional);
        po::store(parsed, var_map);
        notify(var_map);
        help_flag = var_map.count("help");


    } catch (std::exception &ex) {
        throw BuiltInOptionsParseException(ex.what()); // Convert to our error type
    }
}
