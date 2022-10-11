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

#include "options_parser.h"
#include "errors.h"

extern char **environ;

int builtin_command() {
    return 0;
}

int exec_com_line(const std::string &com_line) {
//    відкинути пробіли і решіточку
    const auto str_begin = com_line.find_first_not_of(' ');
    const auto str_end = com_line.find_first_of('#');
    const auto str_range = str_end - str_begin;
    std::string clean_com_line = com_line.substr(str_begin, str_range);
    std::cout << clean_com_line << '\n';
//    розбити по пробілах
    std::stringstream streamData(clean_com_line);
    std::vector<std::string> args;
    std::string value;
    while (std::getline(streamData, value, ' ')) {
        args.push_back(value);
    }
    for (auto &val: args) {
        std::cout << val << std::endl;
    }
//    перевірка на вбудовані команди - виконати
    if (args[0] == "mycat") {
        builtin_command();
    }
//    ні - ім’я команди - викликає форк-екзек, формувати блоки в елсе
    else {
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
            // TODO: fix incorrect status. What is status at all?
            if (status != 0) {
                exit(500);
            }
        }
        else {
            std::string child_name = args[0];

            std::vector<const char*> args_for_exec;
            for (const auto &str: args) {
                args_for_exec.push_back(str.c_str());
            }
            args_for_exec.push_back(nullptr);

            auto path_ptr = getenv("PATH");
            string path_var;
            if(path_ptr != nullptr)
                path_var = path_ptr;
            path_var += ":.";
            setenv("PATH", path_var.c_str(), 1);

            execvp(child_name.c_str(), const_cast<char *const *>(args_for_exec.data()));

            perror("Execve failed");
            exit(errno);
        }
    }
}

void exec_com_lines(std::basic_istream<char> &com_stream) {
    std::string com_line;
    // don't use cin: 1. can't use later, 2. reads till ' '
    while (std::getline(com_stream, com_line)) {
        exec_com_line(com_line);
    }
}

int main(int argc, char* argv[]) {
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
        std::cout << "$ ";

        // TODO: ask about multiline commands
        std::string com_line;
        std::getline(std::cin, com_line);

        exec_com_line(com_line);
    }

    return 0;
}
