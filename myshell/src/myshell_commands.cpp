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
#include <cassert>

#include <readline/readline.h>
#include <readline/history.h>
#include <boost/filesystem.hpp>

#include "options_parser.h"
#include "builtin_parsers/builtin_parser.h"
#include "myshell_exit_codes.h"
#include "myshell_exceptions.h"
#include "myshell_commands.h"
#include "builtin_parsers/merrno_parser.h"
#include "builtin_parsers/mexport_parser.h"
#include "builtin_parsers/mexit_parser.h"
#include "builtin_parsers/mpwd_parser.h"
#include "builtin_parsers/mecho_parser.h"
#include "builtin_parsers/mcd_parser.h"
#include "builtin_parsers/dot_parser.h"

namespace fs = boost::filesystem;

static int exit_status = 0;

/**
 * Run builtin command if it is a builtin command.
 * @param vector of strings - arguments (with command name included)
 * @return whether the command was a built-in command
 */
bool run_builtin_command(std::vector<std::string> &tokens) {
    std::unique_ptr<builtin_parser_t> builtinParser;
    const std::string &command = tokens[0];
    try {
        if (command == "merrno") {
            builtinParser = std::make_unique<merrno_parser_t>();
        } else if (command == "mexport") {
            builtinParser = std::make_unique<mexport_parser_t>();
        } else if (command == "mexit") {
            builtinParser = std::make_unique<mexit_parser_t>();
        } else if(command == "mpwd") {
            builtinParser = std::make_unique<mpwd_parser_t>();
        } else if(command == "mecho") {
            builtinParser = std::make_unique<mecho_parser_t>();
        } else if(command == "mcd") {
            builtinParser = std::make_unique<mcd_parser_t>();
        } else if(command == ".") {
            builtinParser = std::make_unique<dot_parser_t>();
        } else {
            return false;
        }
        builtinParser->setup_description();
        std::vector<std::string> arguments(tokens.begin() + 1, tokens.end());
        builtinParser->parse(arguments);
    } catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        exit_status = ExitCodes::EOTHER;
        return true;
    }

    if (builtinParser->get_help_flag()) {
        builtinParser->print_help_message();
        exit_status = 0;
        return true;
    }

    if (command == "merrno") {
        std::cout << exit_status << std::endl;
        exit_status = 0;
    } else if (command == "mexport") {
        mexport_parser_t &merrno_parser = dynamic_cast<mexport_parser_t &>(*builtinParser);
        const std::vector<std::string> &assignments = merrno_parser.get_assignments();

        for (const std::string &assignment : assignments) { // TODO: check without '='
            const auto str_eq = assignment.find_first_of('=');
            if (str_eq == std::string::npos) continue;

            std::string varname = assignment.substr(0, str_eq);
            std::string val = assignment.substr(str_eq + 1, assignment.size());
            int status = setenv(varname.c_str(), val.c_str(), 1);
            if (status == -1) {
                perror("Failed to set variable");
                exit_status = EFAILSET;
            }
            val = "";
        }
        exit_status = 0;
    } else if(command == "mexit") {
        mexit_parser_t &mexit_parser = dynamic_cast<mexit_parser_t &>(*builtinParser);
        exit(mexit_parser.get_code());
    } else if(command == "mpwd") {
        std::cout << fs::current_path().string() << std::endl;
        exit_status = 0;
    } else if(command == "mecho") {
        mecho_parser_t &mecho_parser = dynamic_cast<mecho_parser_t &>(*builtinParser);
        const std::vector<std::string> &texts = mecho_parser.get_texts();
        for (size_t i = 0; i < texts.size() - 1; ++i) {
            std::cout << texts[i] << " ";
        }
        std::cout << texts.back() << std::endl;
        exit_status = 0;
    } else if(command == "mcd") {
        mcd_parser_t &mcd_parser = dynamic_cast<mcd_parser_t &>(*builtinParser);
        const std::string &path = mcd_parser.get_path();
        if (std::filesystem::is_directory(path)) {
            std::filesystem::current_path(path);
        }
        else if(path == "~") {
            auto path_ptr = getenv("HOME");
            if (path_ptr != nullptr)
                std::filesystem::current_path(path_ptr);
        }
        else{
            exit_status = ENOTADIR;
            std::cerr << "mcd: not a directory: " << path << std::endl;
        }
    } else if(command == ".") {
        dot_parser_t &dot_parser = dynamic_cast<dot_parser_t &>(*builtinParser);
        const std::string &path = dot_parser.get_path();
        if(std::filesystem::is_regular_file(path)) {
            std::ifstream script_file(path);
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
    } else {
        assert(false && "This should not execute");
    }
    return true;
}

std::string remove_spaces(const std::string &com_line) {
    // remove leading spaces and comment
    auto str_begin = com_line.find_first_not_of(' ');
    if (str_begin == std::string::npos) {
        return "";
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

    return clean_com_line;
}

std::vector<std::string> get_matched_filenames(std::string value) {
    std::vector<std::string> matched_filenames;

    fs::path wildc_file_path{value};

    // set searching path
    fs::path wildc_parent_path{"."};
    if (wildc_file_path.has_parent_path()) {
        wildc_parent_path = wildc_file_path.parent_path();
    }

    // check for existence
    if (!fs::exists(wildc_parent_path)) {
        matched_filenames.push_back(value);
        return matched_filenames;
    }

    // iterate over path
    std::string wildc_filename = wildc_file_path.filename().string();
    fs::path cur_file_path;
    for (const auto &entry: fs::directory_iterator(wildc_parent_path)) {
        // compare file name and regex
        cur_file_path = entry.path();

        int res;
        res = fnmatch(wildc_filename.c_str(), cur_file_path.filename().c_str(), FNM_PATHNAME | FNM_PERIOD);

        if (res == 0) {
            matched_filenames.push_back(cur_file_path.string());
        } else if (res != FNM_NOMATCH) {
            std::string error_str{strerror(errno)};
            throw fnmatch_error{"Error in globbing: " + error_str};
        }
    }

    return matched_filenames;
}

std::string expand_variables(std::string value) {
    auto var_begin = value.find_first_of('$');
    if (var_begin != std::string::npos) {
        var_begin++;

        // replace env variables
        auto var_ptr = getenv(value.substr(var_begin, value.size() - var_begin).c_str());
        std::string var_val;
        if (var_ptr != nullptr) {
            var_val = var_ptr;
        }
        value = value.substr(0, var_begin - 1) + var_val;
    }

    return value;
}

std::vector<std::string> parse_com_line(const std::string &com_line) {
    std::vector<std::string> args;

    std::string clean_com_line = remove_spaces(com_line);
    if (clean_com_line.empty()) {
        return args;
    }

    // split by space and expand
    std::stringstream streamData(clean_com_line);
    size_t arg_num = 0;
    std::string value;
    while (std::getline(streamData, value, ' ')) {
        // expand variable if exists
        value = expand_variables(value);

        if (arg_num == 0) {
            args.push_back(value);
        }
        else {
            // replace with wildcard
            std::vector<std::string> matched_filenames = get_matched_filenames(value);
            // add to args
            if (!matched_filenames.empty()) {
                args.insert(args.end(), matched_filenames.begin(), matched_filenames.end());
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
        exit_status = ExitCodes::EFORKFAIL;
        return;
    }

    if (pid != 0) {
        int child_status;
        while (true) {
            int status = waitpid(pid, &child_status, 0);
            if (status == -1) {
                if (errno != EINTR) {
                    perror("Unexpected error from waitpid");
                    exit_status = ExitCodes::EOTHER;
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
            exit_status = ExitCodes::ESIGNALFAIL;
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
        exit(ExitCodes::EEXECFAIL);
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
