//
// Created by vivi on 12.11.22.
//

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <cstring>

using cmd_args = std::vector<std::string>;

void close_other_pipes(int cmd_idx, std::vector<int> pipes_fds) {
    for (int i = 0; i < pipes_fds.size(); i++) {
        if (i != 2 * cmd_idx - 1 && i != 2 * cmd_idx) {
            if (close(pipes_fds[i]) == -1) {
                char* error_info;
                sprintf(error_info, "command %d, other pipes closing: ", cmd_idx);
                throw std::runtime_error{strcat(error_info, strerror(errno))};
            }
        }
    }
    // need std::endl to flush buffer before changing output to pipe end
}

void change_command_pipe_streams(int command_idx, int commands_num, std::vector<int> &pipes_fds) {
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

void exec_shell_line(std::vector<cmd_args> &cmds_args) {
    std::vector<int> pipes_fds((cmds_args.size() - 1) * 2);

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

    for (int cmd_idx = 0; cmd_idx < cmds_args.size(); cmd_idx++) {
        pid_t pid = fork();

        if (pid == 0) {
            close_other_pipes(cmd_idx, pipes_fds);

            change_command_pipe_streams(cmd_idx, cmds_args.size(), pipes_fds);

            std::string &file_for_exec = cmds_args[cmd_idx][0];
            cmd_args &args = cmds_args[cmd_idx];

            execute_command(file_for_exec, args);
        }
    }

    close_other_pipes(-1, pipes_fds);

    for (int i = 0; i < cmds_args.size(); i++) {
        if (waitpid(-1, nullptr, 0) == -1) {
            perror("wait");
        }
    }
}

cmd_args split_cmd_line(std::string &cmd_line) {
    std::stringstream streamData(cmd_line);
    std::string value;
    cmd_args args;
    while (std::getline(streamData, value, ' ')) {
        args.push_back(value);
    }
    return args;
}


std::vector<cmd_args> split_shell_line(std::string &shell_line) {
    size_t begin = 0, end;
    std::string delim = "|";
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
    for (std::string &line: cmd_lines) {
        cmd_args args = split_cmd_line(line);
        cmds_args.push_back(args);
    }

    return cmds_args;
}

std::vector<std::string> split_cmd_and_redirection(std::string &shell_line) {
    size_t begin = 0, end;
    std::string delim = ">";
    std::vector<std::string> cmd_lines;
    std::string cmd_line;
    while ((end = shell_line.find(delim, begin)) != std::string::npos) {
        if (end != 0) {

        }
        cmd_line = shell_line.substr(begin, end - begin);

        cmd_lines.push_back(cmd_line);
        begin = end + delim.length();
    }
    cmd_line = shell_line.substr(begin, end);
    cmd_lines.push_back(cmd_line);

    std::vector<cmd_args> cmds_args;
    for (std::string &line: cmd_lines) {
        cmd_args args = split_cmd_line(line);
        cmds_args.push_back(args);
    }

    return cmd_lines;
}



int main() {
    // TODO:

    std::string command_line = "echo 123 456 | wc | wc > text 2>&1";

//    std::vector<std::string> cmd_and_redirection = split_shell_line(command_line, ">");

    for (const auto& elm : cmd_and_redirection) {
        std::cout << elm << std::endl;
    }

//    std::cout << cmd_and_redirection[0] << std::endl;

//    std::vector<cmd_args> cmds_args = split_shell_line(cmd_and_redirection[0], "|");

//    exec_shell_line(cmds_args);
}