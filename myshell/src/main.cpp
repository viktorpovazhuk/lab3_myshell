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
#include <filesystem>

#include "options_parser.h"
#include "errors.h"

namespace fs = std::filesystem;

extern char **environ;

void run_builtin_command(std::vector<std::string> &args) {

}

std::vector<std::string> parse_com_line(const std::string &com_line) {
    // remove leading spaces and comment
    const auto str_begin = com_line.find_first_not_of(' ');
    const auto str_end = com_line.find_first_of('#');
    const auto str_range = str_end - str_begin;
    std::string clean_com_line = com_line.substr(str_begin, str_range);
    std::cout << clean_com_line << '\n';

    // split by space
    std::stringstream streamData(clean_com_line);
    std::vector<std::string> args;
    std::string value;
    while (std::getline(streamData, value, ' ')) {
        // substitute env vars
        if (value[0] == '$') {
            auto var_ptr = getenv(value.substr(1, value.size() - 1).c_str());
            std::string var_val;
            if (var_ptr != nullptr) {
                var_val = var_ptr;
            }
            value = var_val;
            args.push_back(value);
        } else if (nullptr) { // replace if wildcard

        } else {
            args.push_back(value);
        }
    }
    for (auto &val: args) {
        std::cout << val << std::endl;
    }

    return std::move(args);
}

void run_outer_command(std::vector<std::string> &args) {
    pid_t pid = fork();

    while (pid == -1) {
        if (errno == EINTR) {
            pid = fork();
            continue;
        } else {
            perror("Fork failed");
            exit(errno);
        }
    }

    if (pid != 0) {
        int status;
        waitpid(pid, &status, 0);
        // TODO:
        //  1. What is status at all?
        //  2. What to do is command was incorrect: continue or exit? Continue, but main process prints info before '$'.
    } else {
        std::string child_name = args[0];

        std::vector<const char *> args_for_exec;
        for (const auto &str: args) {
            args_for_exec.push_back(str.c_str());
        }
        args_for_exec.push_back(nullptr);

        auto path_ptr = getenv("PATH");
        string path_var;
        if (path_ptr != nullptr)
            path_var = path_ptr;
        // TODO: add absolute directory not to execute script as a command?
        path_var += ":.";
        setenv("PATH", path_var.c_str(), 1);

        execvp(child_name.c_str(), const_cast<char *const *>(args_for_exec.data()));

        // bug of CLion: reads firstly from cout, then from cerr
        // even if buffers are flushed
        perror("Execve failed");
        exit(errno);
    }
}

void exec_com_line(const std::string &com_line) {
    std::vector<std::string> args = parse_com_line(com_line);
    // check for in-built command
    if (args[0] == "mycat") {
        run_builtin_command(args);
    }
        // check for script
    else if (args.size() == 1 && fs::path{args[0]}.extension() == ".msh" && fs::exists(fs::path{args[0]})
             && fs::path{args[0]}.has_parent_path()) { // check for existence of path
        fs::path script_path{args[0]};
        args.push_back(args[0]);
        args[0] = "myshell";
        run_outer_command(args);
    }
        // run fork-exec
    else {
        run_outer_command(args);
    }
}

void exec_com_lines(std::basic_istream<char> &com_stream) {
    std::string com_line;
    // don't use cin: 1. can't use later, 2. reads till ' '
    while (std::getline(com_stream, com_line)) {
        exec_com_line(com_line);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        std::unique_ptr<command_line_options_t> command_line_options;
        try {
            command_line_options = std::make_unique<command_line_options_t>(argc, argv);
        }
        catch (std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            exit(Errors::ECLOPTIONS);
        }
        std::string script_path = command_line_options->script_path;

        std::ifstream script_file(script_path);
        exec_com_lines(script_file);

        exit(EXIT_SUCCESS);
    }

    while (true) {
        // TODO: add wd with created pwd() function
        std::cout << "$ ";

        std::string com_line;
        std::getline(std::cin, com_line);

        exec_com_line(com_line);
    }

    return 0;
}
