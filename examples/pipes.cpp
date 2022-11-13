//
// Created by vivi on 12.11.22.
//

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

using cmd_args = std::vector<std::string>;

void close_other_pipes(int cmd_idx, std::vector<int> pipes_fds) {
    std::cout << "cmd idx in close_other_pipes: " << cmd_idx << '\n';
    for (int i = 0; i < pipes_fds.size(); i++) {
        if (i != 2 * cmd_idx - 1 && i != 2 * cmd_idx) {
            std::cout << "closed idx: " << i << ", closed fd: " << pipes_fds[i] << '\n';
            close(pipes_fds[i]);
        }
    }
    // need std::endl to flush buffer before changing output to pipe end
    std::cout << "end of close_other_pipes !!!" << std::endl;
}

void change_command_pipe_streams(int command_idx, int commands_num, std::vector<int> &pipes_fds) {
    if (command_idx != 0) {
        if (dup2(pipes_fds[command_idx*2-1], STDIN_FILENO) == -1) {
            perror("dup1");
        }
        if (close(pipes_fds[command_idx*2-1]) == -1) {
            perror("close1");
        }
    }
    if (command_idx != commands_num - 1) {
        if (dup2(pipes_fds[command_idx*2], STDOUT_FILENO) == -1) {
            perror("dup2");
        }
        if (close(pipes_fds[command_idx*2]) == -1) {
            perror("close2");
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

void exec_com_line(std::vector<cmd_args> &cmds_args) {
    std::vector<int> pipes_fds((cmds_args.size() - 1) * 2);

    for (int i = 0; i < pipes_fds.size() / 2; i++) {
        int fds[2];
        pipe(fds);
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

int main() {
    std::vector<cmd_args> cmds_args;
    cmds_args.push_back(cmd_args{"echo", "123 456"});
    cmds_args.push_back(cmd_args{"wc"});
    cmds_args.push_back(cmd_args{"wc"});

    exec_com_line(cmds_args);
}