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
#include <fcntl.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <boost/filesystem.hpp>
#include <unordered_set>

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
using cmd_args = std::vector<std::string>;

static int exit_status = 0;

static int link_fd_to_stdout(int fd) {
    if (fd == STDOUT_FILENO)
        return STDOUT_FILENO;
    int dup_stdout;
    while (true) {
        dup_stdout = dup(STDOUT_FILENO);
        if (dup_stdout == -1) {
            if (errno != EINTR) {
                perror("dup");
                return -1;
            }
        } else {
            break;
        }
    }

    while (true) {
        int status = dup2(fd, STDOUT_FILENO);
        if (status == -1) {
            if (errno != EINTR) {
                perror("dup2");
                return -1;
            }
        } else {
            break;
        }
    }
    return dup_stdout;
}

static int undo_link_fd_to_stdout(int old_stdout) {
    if (old_stdout == STDOUT_FILENO)
        return 1;
    while (true) {
        int status = dup2(old_stdout, STDOUT_FILENO);
        if (status == -1) {
            if (errno != EINTR) {
                perror("dup2");
                return -1;
            }
        } else {
            return 0;
        }
    }
}

/**
 * Run builtin command if it is a builtin command.
 * @param vector of strings - arguments (with command name included)
 * @return whether the command was a built-in command
 */
bool run_builtin_command(std::vector<std::string> &tokens, int fd_out) {
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

    int dup_stdout = link_fd_to_stdout(fd_out);

    if (builtinParser->get_help_flag()) {
        builtinParser->print_help_message();
        exit_status = 0;
        undo_link_fd_to_stdout(dup_stdout);
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
                exec_shell_lines(script_file);
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
    undo_link_fd_to_stdout(dup_stdout);
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

void close_other_pipes(int cmd_idx, int commands_num, std::vector<int> pipes_fds) {
    for (int i = 0; i < pipes_fds.size(); i++) {
        if (i != 2 * cmd_idx - 1 && i != 2 * cmd_idx && !(cmd_idx == 0 && i == pipes_fds.size() - 2) && !(cmd_idx == commands_num - 1 && i == pipes_fds.size() - 1)) {
            if (close(pipes_fds[i]) == -1) {
                char* error_info;
                sprintf(error_info, "command %d, other pipes closing: ", cmd_idx);
                throw std::runtime_error{strcat(error_info, strerror(errno))};
            }
        }
    }
    // need std::endl to flush buffer before changing output to pipe end
}

void change_command_streams(int command_idx, int commands_num, std::vector<int> &pipes_fds) {
    if (command_idx != 0) {
        if (dup2(pipes_fds[command_idx*2-1], STDIN_FILENO) == -1) {
            char* error_info;
            sprintf(error_info, "command %d, stdin substitution: ", command_idx);
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
        if (close(pipes_fds[command_idx*2-1]) == -1) {
            char* error_info;
            sprintf(error_info, "command %d, stdin pipe end close: ", command_idx);
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
    }
    else if (command_idx == 0 && pipes_fds[pipes_fds.size() - 2] != STDIN_FILENO) {
        int input_fd = pipes_fds[pipes_fds.size() - 2];
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            char* error_info;
            sprintf(error_info, "command 0, stdin substitution: ");
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
        if (close(input_fd) == -1) {
            char* error_info;
            sprintf(error_info, "command 0, stdin pipe end close: ");
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
    }
    if (command_idx != commands_num - 1) {
        if (dup2(pipes_fds[command_idx*2], STDOUT_FILENO) == -1) {
            char* error_info;
            sprintf(error_info, "command %d, stdout substitution: ", command_idx);
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
        if (close(pipes_fds[command_idx*2]) == -1) {
            char* error_info;
            sprintf(error_info, "command %d, stdout pipe end close: ", command_idx);
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
    }
    else if (command_idx == commands_num - 1 && pipes_fds[pipes_fds.size() - 1] != STDOUT_FILENO) {
        int output_fd = pipes_fds[pipes_fds.size() - 1];
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            char* error_info;
            sprintf(error_info, "last command, stdout substitution: ");
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
        if (close(output_fd) == -1) {
            char* error_info;
            sprintf(error_info, "last command, stdout pipe end close: ");
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
    }
}

void execute_command(std::string &file_for_exec, cmd_args &args) {
    std::vector<const char *> args_for_exec;
    for (const auto &str: args) {
        args_for_exec.push_back(str.c_str());
    }
    args_for_exec.push_back(nullptr);

    execvp(file_for_exec.c_str(), const_cast<char *const *>(args_for_exec.data()));
    perror("Exec failed");
}

void exec_piped_commands(std::vector<cmd_args> &cmds_args, int input_fd, int output_fd) {
    std::vector<int> pipes_fds((cmds_args.size() - 1) * 2);
    int commands_num = cmds_args.size();

    for (int i = 0; i < pipes_fds.size() / 2; i++) {
        int fds[2];
        if (pipe(fds) == -1) {
            char* error_info;
            sprintf(error_info, "pipe %d, pipe creation: ", i);
            throw std::runtime_error{strcat(error_info, strerror(errno))};
        }
        pipes_fds[i*2] = fds[1];
        pipes_fds[i*2+1] = fds[0];
    }
    pipes_fds.push_back(input_fd);
    pipes_fds.push_back(output_fd);

    for (int cmd_idx = 0; cmd_idx < commands_num; cmd_idx++) {
        pid_t pid = fork();

        if (pid == 0) {
            close_other_pipes(cmd_idx, commands_num, pipes_fds);

            change_command_streams(cmd_idx, commands_num, pipes_fds);

            std::string &file_for_exec = cmds_args[cmd_idx][0];
            cmd_args &args = cmds_args[cmd_idx];

            execute_command(file_for_exec, args);
        }
    }

    close_other_pipes(-1, commands_num, pipes_fds);

    for (int i = 0; i < commands_num; i++) {
        if (waitpid(-1, nullptr, 0) == -1) {
            perror("wait");
        }
    }
}

std::vector<cmd_args> split_shell_line(std::string &shell_line) {
    size_t begin = 0, end;
    std::string delim = " | ";
    std::vector<std::string> cmd_lines;
    std::string cmd_line;
    while ((end = shell_line.find(delim, begin)) != std::string::npos) {
        cmd_line = shell_line.substr(begin, end - begin);

        cmd_lines.push_back(cmd_line);
        begin = end + delim.length();
    }
    cmd_line = shell_line.substr(begin, end);
    cmd_lines.push_back(cmd_line);

    std::vector<cmd_args> cmds_args;
    for (const std::string &line: cmd_lines) {
        cmd_args args = parse_com_line(line);
        cmds_args.push_back(args);
    }

    return cmds_args;
}

/**
 * @param string
 * @returns whether the passed string is the name of the builtin command.
 */
static bool check_builtin(const std::string &s) {
    std::unordered_set<std::string> builtins{"merrno", "mexport", "mexit", "mpwd", "mecho", "mcd", "."};
    return builtins.count(s);
}

void exec_shell_line(std::string &shell_line) {
    std::vector<cmd_args> cmds_args = split_shell_line(shell_line);

    std::vector<pid_t> child_pids;

    int dup_stdout = dup(STDOUT_FILENO);
    while (cmds_args.size() > 1) {
        cmd_args cur_command_line = cmds_args.back(); cmds_args.pop_back();
        int pfd[2];
        pipe(pfd);
        pid_t child_pid = fork();
        if (child_pid == 0) {
            close(dup_stdout);
            close(pfd[1]);
            dup2(pfd[0], STDIN_FILENO);
            close(pfd[0]);

//            ...configure redirections

            execute_command(cur_command_line[0], cur_command_line);
        }
        child_pids.push_back(child_pid);
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
    }
    auto cur_command_line = cmds_args.back();
    if (check_builtin(cur_command_line[0])) {
//        ...configure redirections
        run_builtin_command(cur_command_line, STDOUT_FILENO);
    } else {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            close(dup_stdout);
//            ...configure redirections
            execute_command(cur_command_line[0], cur_command_line);
        }
        child_pids.push_back(child_pid);
    }
    dup2(dup_stdout, STDOUT_FILENO);
    close(dup_stdout);

    for (pid_t child_pid : child_pids) {
        int child_status;
        waitpid(child_pid, &child_status, 0);
    }
}

void exec_shell_lines(std::basic_istream<char> &com_stream) {
    std::string com_line;
    // don't use cin: 1. can't use later, 2. reads till ' '
    while (std::getline(com_stream, com_line)) {
        exec_shell_line(com_line);
    }
}

std::string get_prompt() {
    return fs::current_path().string() + " $ ";
}
