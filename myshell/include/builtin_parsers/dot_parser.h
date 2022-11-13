#ifndef MYSHELL_DOT_PARSER_H
#define MYSHELL_DOT_PARSER_H


#include "builtin_parser.h"

class dot_parser_t : public builtin_parser_t {
public:
    dot_parser_t() = default;
    dot_parser_t(const dot_parser_t &) = default;
    dot_parser_t &operator=(const dot_parser_t &) = delete;
    dot_parser_t(dot_parser_t &&) = default;
    dot_parser_t &operator=(dot_parser_t &&) = delete;
    ~dot_parser_t() = default;

    virtual void print_help_message() override;
    virtual void parse(const std::vector<std::string> &av) override;

    const std::string &get_path() {
        return path;
    }
protected:
    std::string path;
};


#endif //MYSHELL_DOT_PARSER_H
