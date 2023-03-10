//
// Created by Bohdan Shevchuk on 16.10.2022.
//

#ifndef MYSHELL_MYSHELL_COMMANDS_H
#define MYSHELL_MYSHELL_COMMANDS_H

bool run_builtin_command(std::vector<std::string> &args);

std::vector<std::string> parse_com_line(const std::string &com_line);

void run_outer_command(std::vector<std::string> &args);

void exec_com_line(const std::string &com_line);

void exec_com_lines(std::basic_istream<char> &com_stream);

std::string get_prompt();



#endif //MYSHELL_MYSHELL_COMMANDS_H
