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
#include "myshell_commands.h"

namespace fs = boost::filesystem;

static int exit_status = 0;

bool run_builtin_command(std::vector<std::string> &args) {
    std::unique_ptr<com_line_built_in> commandLineOptions;
    try {
        commandLineOptions = std::make_unique<com_line_built_in>(args.size(), args);
    }
    catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return true;
    }


    std::vector<std::string> parsed_args = commandLineOptions->get_filenames();


    if (commandLineOptions->get_help_flag()) {
        std::cout << commandLineOptions->get_help_msg() << std::endl;
        return true;
    }


    if (parsed_args[0] == "merrno") {
        std::cout << exit_status << std::endl;
        exit_status = 0;
        if (parsed_args.size() > 1)
            exit_status = EWRONGPARAMS;
    }

    else if (parsed_args[0] == "mexport") {
        exit_status = 0;
        if (parsed_args.size() > 1)
            for (size_t i = 1; i < parsed_args.size(); ++i) {
                const auto str_eq = parsed_args[i].find_first_of('=');

                std::string varname = parsed_args[i].substr(0, str_eq);
                std::string val = parsed_args[i].substr(str_eq + 1, parsed_args[i].size());
                int status = setenv(varname.c_str(), val.c_str(), 1);
                if (status == -1) {
                    perror("Failed to set PATH variable");
                    exit_status = EFAILSET;
                }

            }
    }
    else if(parsed_args[0] == "mexit") {
        int exit_status = 0;
        if (parsed_args.size() > 1) {
            exit_status = atoi(parsed_args[1].c_str());
        }
        exit(exit_status);
    }

    else if(parsed_args[0] == "mpwd") {
        if (parsed_args.size() > 1) {
            std::cerr << "mpwd does not take parameters" << std::endl;
            exit_status = EWRONGPARAMS;
        }
        else
            std::cout << fs::current_path().string() << std::endl;
    }
    else if(parsed_args[0] == "mecho") {
        if(parsed_args.size() > 1) {
            for(size_t i = 1; i < parsed_args.size() - 1; ++i) {
                std::cout << parsed_args[i] << " ";
            }
            std::cout << parsed_args[parsed_args.size() - 1] << std::endl;
        }
        exit_status = 0;
    }
    else if(parsed_args[0] == "mcd") {
        if(parsed_args.size() != 2) {
            exit_status = EWRONGPARAMS;
            std::cerr << "mcd takes only one parameter - path to change directory." << std::endl;
        }
        else{
            if(std::filesystem::is_directory(parsed_args[1])) {
                std::filesystem::current_path(parsed_args[1]);
            }
            else if(parsed_args[1] == "~") {
                auto path_ptr = getenv("HOME");
                if (path_ptr != nullptr)
                    std::filesystem::current_path(path_ptr);

            }
            else{
                exit_status = ENOTADIR;
                std::cerr << "mcd: not a directory: " << parsed_args[1] << std::endl;
            }

        }
    }

    else if(parsed_args[0] == ".") {
        if(std::filesystem::is_regular_file(parsed_args[1])) {
            std::ifstream script_file(parsed_args[1]);
            try {
                exec_com_lines(script_file);
            } catch (std::exception &ex) {
                std::cerr << ex.what() << '\n';
                exit_status = EXIT_FAILURE;
            }
        }
        else {
            std::cerr << "coud not find necessary script" << std::endl;
            exit_status = ENOTASCRIPT;
        }
    }
    else
        return false;

    return true;
}

std::vector<std::string> parse_com_line(const std::string &com_line) {
    std::vector<std::string> args;

    // remove leading spaces and comment
    auto str_begin = com_line.find_first_not_of(' ');
    if (str_begin == std::string::npos) {
        return args;
    }
    auto str_end = com_line.find_first_of('#');
    if (str_end == std::string::npos) {
        str_end = com_line.size();
    }
    auto str_range = str_end - str_begin;
    std::string clean_com_line = com_line.substr(str_begin, str_range);

    // remove multiple spaces
    size_t pos;
    while((pos = clean_com_line.find("  ")) != std::string::npos)
    {
        clean_com_line.replace(pos, 2, " ");
    }

    // split by space and expand
    std::stringstream streamData(clean_com_line);
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
        } else { // replace as wildcard
            fs::path wildc_file_path{value};
            // set searching path
            fs::path wildc_parent_path{"."};
            if (wildc_file_path.has_parent_path()) {
                wildc_parent_path = wildc_file_path.parent_path();
            }
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
        }

        arg_num += 1;
    }

    return args;
}

void run_outer_command(std::vector<std::string> &args) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit_status = Errors::EFORKFAIL;
        return;
    }

    if (pid != 0) {
        int child_status;
        while (true) {
            int status = waitpid(pid, &child_status, 0);
            if (status == -1) {
                if (errno != EINTR) {
                    perror("Unexpected error from waitpid");
                    exit_status = Errors::EOTHER;
                    return;
                }
            } else {
                break;
            }
            // here EINTR happened
        }
        if (WIFEXITED(child_status)) {
            exit_status = WEXITSTATUS(child_status);
        } else if (WIFSIGNALED(child_status)) {
            exit_status = Errors::ESIGNALFAIL;
        }
    } else {
        std::string file_for_exec;
        std::vector<const char *> args_for_exec;

        if (fs::path{args[0]}.extension() == ".msh") {
            file_for_exec = "myshell";
            args_for_exec.push_back("myshell");
        } else {
            file_for_exec = args[0];
        }

        for (const auto &str: args) {
            args_for_exec.push_back(str.c_str());
        }
        args_for_exec.push_back(nullptr);

        execvp(file_for_exec.c_str(), const_cast<char *const *>(args_for_exec.data()));
        perror("Exec failed");
        exit(Errors::EEXECFAIL);
    }
}

void exec_com_line(const std::string &com_line) {
    std::vector<std::string> args = parse_com_line(com_line);
    if (args.empty()) {
        return;
    }
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
