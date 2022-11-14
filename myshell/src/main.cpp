// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory>
#include <fstream>
#include <regex>
#include <fnmatch.h>
#include <utility>
#include <cstdlib>

#include <readline/readline.h>
#include <readline/history.h>
#include <boost/filesystem.hpp>

#include "options_parser.h"
#include "builtin_parsers/builtin_parser.h"
#include "myshell_exit_codes.h"
#include "myshell_exceptions.h"
#include "myshell_commands.h"

namespace fs = boost::filesystem;

int main(int argc, char *argv[]) {
    signal(SIGCHLD, SIG_IGN);
    if (argc > 1) {
        std::unique_ptr<command_line_options_t> command_line_options;
        try {
            command_line_options = std::make_unique<command_line_options_t>(argc, argv);
        }
        catch (std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        std::string script_path = command_line_options->script_path;

        std::ifstream script_file(script_path);
        try {
            exec_shell_lines(script_file);
        } catch (std::exception &ex) {
            std::cerr << ex.what() << '\n';
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }

    // add paths to shell and outer commands
    auto path_ptr = getenv("PATH");
    string path_var;
    if (path_ptr != nullptr)
        path_var = path_ptr;
    else
        path_var = "";
    if (!path_var.empty())
        path_var = ":" + path_var;
    fs::path parent_dir = fs::system_complete(argv[0]).parent_path();
    path_var = parent_dir.string() + ":" + (parent_dir / fs::path{"utils"}).string() + path_var;
    int status = setenv("PATH", path_var.c_str(), 1);
    if (status == -1) {
        perror("Failed to set PATH variable");
        exit(EXIT_FAILURE);
    }

    char *shell_line;
    while ((shell_line = readline(get_prompt().c_str())) != nullptr) {
        std::string shell_line_str{shell_line};
        if (!shell_line_str.empty()) {
            add_history(shell_line);
        }
        try {
            exec_shell_line(shell_line_str);
        } catch (std::exception &ex) {
            std::cerr << ex.what() << '\n';
        }
        free(shell_line);
    }

    return 0;
}
