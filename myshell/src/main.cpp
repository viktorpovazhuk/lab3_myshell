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
#include "built_in_parser.h"
#include "myshell_errors.h"
#include "myshell_exceptions.h"

namespace fs = boost::filesystem;

extern char **environ;

int myerrnum = 0;

bool run_builtin_command(std::vector<std::string> &args) {
    std::unique_ptr<com_line_built_in> commandLineOptions;
    try {
        commandLineOptions = std::make_unique<com_line_built_in>(args.size(), args);
    }
    catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return true;
        exit(Errors::ECLOPTIONS);
    }


    std::vector<std::string> parsed_args = commandLineOptions->get_filenames();


    if (commandLineOptions->get_help_flag()) {
        std::cout << commandLineOptions->get_help_msg() << std::endl;
        return true;
    }


    if (parsed_args[0] == "merrno") {
        std::cout << myerrnum << std::endl;
        myerrnum = 0;
        if (parsed_args.size() > 1)
            myerrnum = EWRONGPARAMS;
    }

    else if (parsed_args[0] == "mexport") {
        myerrnum = 0;
        if (parsed_args.size() > 1)
            for (size_t i = 1; i < parsed_args.size(); ++i) {
                const auto str_eq = parsed_args[i].find_first_of('=');

                std::string varname = parsed_args[i].substr(0, str_eq);
                std::string val = parsed_args[i].substr(str_eq + 1, parsed_args[i].size());
                int status = setenv(varname.c_str(), val.c_str(), 1);
                if (status == -1) {
                    perror("Failed to set PATH variable");
                    myerrnum = EFAILSET;
                }

            }
    }
    else if(parsed_args[0] == "mexit") {
        int exit_status = 0;
        if (parsed_args.size() == 1) {
            exit_status = atoi(parsed_args[1].c_str());
        }
        exit(exit_status);
    }

    else if(parsed_args[0] == "mpwd") {
        if (parsed_args.size() > 1) {
            myerrnum = EWRONGPARAMS;
        }
        else {

        }
    } else {
        return false;
    }

    return true;


}

std::vector<std::string> parse_com_line(const std::string &com_line) {
    // remove leading spaces and comment
    const auto str_begin = com_line.find_first_not_of(' ');
    const auto str_end = com_line.find_first_of('#');
    const auto str_range = str_end - str_begin;
    std::string clean_com_line = com_line.substr(str_begin, str_range);
    std::cout << clean_com_line << '\n';

    // split by space and expand
    std::stringstream streamData(clean_com_line);
    std::vector<std::string> args;
    size_t arg_num = 0;
    std::string value;
    while (std::getline(streamData, value, ' ')) {
        // substitute env vars
        if (arg_num == 0) {
            args.push_back(value);
        } else if (value[0] == '$') { // replace env variables
            auto var_ptr = getenv(value.substr(1, value.size() - 1).c_str());
            std::string var_val;
            if (var_ptr != nullptr) {
                var_val = var_ptr;
            }
            value = var_val;
            args.push_back(value);
        } else if (value.find('/') != std::string::npos) { // replace if wildcard
            fs::path wildc_file_path{value};
            // set searching path
            fs::path wildc_parent_path = wildc_file_path.parent_path();
            // check for existence
            if (!fs::exists(wildc_parent_path)) {
                args.push_back(value);
                continue;
            }
            // iterate over path
            std::string wildc_filename = wildc_file_path.filename().string();
            std::vector<std::string> matched_file_paths;
            fs::path cur_file_path;
            for (const auto &entry: fs::directory_iterator(wildc_parent_path)) {
                // compare file name and regex
                cur_file_path = entry.path();

                int res;
                res = fnmatch(wildc_filename.c_str(), cur_file_path.filename().c_str(), FNM_PATHNAME | FNM_PERIOD);

                if (res == 0) {
                    matched_file_paths.push_back(cur_file_path.string());
                } else if (res != FNM_NOMATCH) {
                    std::string error_str{strerror(errno)};
                    throw fnmatch_error{"Error in globbing: " + error_str};
                }
            }
            // add to args
            if (!matched_file_paths.empty()) {
                args.insert(args.end(), matched_file_paths.begin(), matched_file_paths.end());
            } else {
                args.push_back(value);
            }
        } else {
            args.push_back(value);
        }

        arg_num += 1;
    }
    for (auto &val: args) {
        std::cout << val << std::endl;
    }

    return args;
}

bool run_outer_command(std::vector<std::string> &args) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid != 0) {
        int child_status;
        waitpid(pid, &child_status, 0);
        if (WIFEXITED(child_status)) {
            int exit_status = WEXITSTATUS(child_status);
        }
        else if (WIFSIGNALED(child_status)) {
            int signal_num = WTERMSIG(child_status);
        }
        // TODO: how shell use child exit status?
    } else {
        std::string child_name = args[0];

        std::vector<const char *> args_for_exec;
        for (const auto &str: args) {
            args_for_exec.push_back(str.c_str());
        }
        args_for_exec.push_back(nullptr);

        int status = execvp(child_name.c_str(), const_cast<char *const *>(args_for_exec.data()));

        if (errno == ENOEXEC) {
            // executable in unrecognized format.
            // we should try to run it as shell script instead
            execvp(child_name.c_str(), const_cast<char *const *>(args_for_exec.data()));
            exit(EXIT_FAILURE);
            return true;
        }

        // bug of CLion: reads firstly from cout, then from cerr
        // even if buffers are flushed
        perror("Execve failed");
        exit(errno);
    }
    return false;
}

void exec_com_line(const std::string &com_line) {
    std::vector<std::string> args = parse_com_line(com_line);
    bool is_builtin = run_builtin_command(args);
    if (!is_builtin) {
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

std::string get_prompt() {
    return fs::current_path().string() + " $ ";
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
        try {
            exec_com_lines(script_file);
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

    char *com_line;
    while ((com_line = readline(get_prompt().c_str())) != nullptr) {
        std::string com_line_str{com_line};
        if (com_line_str.size() > 0) {
            add_history(com_line);
        }
        try {
            exec_com_line(com_line);
        } catch (std::exception &ex) {
            std::cerr << ex.what() << '\n';
        }
        free(com_line);
    }

    return 0;
}
