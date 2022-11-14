//
// Created by vivi on 12.11.22.
//

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <cstring>
#include <fcntl.h>

using cmd_args = std::vector<std::string>;

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
    for (std::string &line: cmd_lines) {
        cmd_args args = split_cmd_line(line);
        for (const auto& arg : args) {
//            std::cout << "arg:" << arg << ":arg ";
        }
        cmds_args.push_back(args);
    }

    return cmds_args;
}




std::vector<std::string> split_redirection_and_shell_line(std::string &shell_line) {
    size_t begin = 0, end;
    std::string delim = ">";
    std::vector<std::string> cmd_lines;
    std::string cmd_line;
    while ((end = shell_line.find(delim, begin)) != std::string::npos) {
        if (end == 0) continue;

        cmd_line = shell_line.substr(begin, end - begin);

        cmd_lines.push_back(cmd_line);
        begin = end + delim.length();
    }
    cmd_line = shell_line.substr(begin, end);
    cmd_lines.push_back(cmd_line);

//    std::vector<cmd_args> cmds_args;
//    for (const std::string &line: cmd_lines) {
//        cmd_args args = parse_com_line(line);
//        cmds_args.push_back(args);
//    }

    return cmd_lines;
}


int main() {
    // TODO:

    std::string command_line = "> cat | wc | wc > text whyy";

    std::vector<std::string> v = split_redirection_and_shell_line(command_line);

    for (const auto& line: v) {
        std::cout << line << std::endl;
    }

    std::vector<cmd_args> cmds_args = split_shell_line(command_line);

//    int input_fd = open("intest", O_RDONLY);
//    int output_fd = STDOUT_FILENO; // open("outtest", O_WRONLY);

//    exec_piped_commands(cmds_args, input_fd, output_fd);
}