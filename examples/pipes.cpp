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
}

int main() {
    std::vector<cmd_args> cmds;
    cmds.push_back(cmd_args{"ls", "-l"});
    cmds.push_back(cmd_args{"wc"});
    cmds.push_back(cmd_args{"wc"});

    std::vector<int> pipes_fds((cmds.size() - 1) * 2);

    for (int i = 0; i < pipes_fds.size() / 2; i++) {
        int fds[2];
        pipe(fds);
        pipes_fds[i*2] = fds[1];
        pipes_fds[i*2+1] = fds[0];
    }

    for (int cmd_idx = 0; cmd_idx < cmds.size(); cmd_idx++) {
        pid_t pid = fork();

        if (pid == 0) {
            std::cout << "cmd idx in main: " << cmd_idx << '\n';

            close_other_pipes(cmd_idx, pipes_fds);

            for (int fd: pipes_fds) {
                std::cout << fd << '\n';
            }

            if (cmd_idx != 0) {
                if (dup2(pipes_fds[cmd_idx*2-1], STDIN_FILENO) == -1) {
                    perror("dup1");
                }
                if (close(pipes_fds[cmd_idx*2-1]) == -1) {
                    perror("close1");
                }
            }
            if (cmd_idx != cmds.size() - 1) {
                std::cout << "in 2 pipe" << '\n';
                std::cout << "cout: " << STDOUT_FILENO << '\n';
                if (dup2(pipes_fds[cmd_idx*2], STDOUT_FILENO) == -1) {
                    perror("dup2");
                }
                std::cout << "after dup2" << '\n';
                if (close(pipes_fds[cmd_idx*2]) == -1) {
                    perror("close2");
                }
            }

            std::string file_for_exec = cmds[cmd_idx][0];

            std::vector<const char *> args_for_exec;
            for (const auto &str: cmds[cmd_idx]) {
                args_for_exec.push_back(str.c_str());
            }
            args_for_exec.push_back(nullptr);

            execvp(file_for_exec.c_str(), const_cast<char *const *>(args_for_exec.data()));
            perror("Exec failed");
        }
    }

    close_other_pipes(-1, pipes_fds);

    for (int i = 0; i < cmds.size(); i++) {
        if (waitpid(-1, nullptr, 0) == -1) {
            perror("wait");
        }
    }
}