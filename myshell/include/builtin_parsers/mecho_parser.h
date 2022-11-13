#ifndef MYSHELL_MECHO_PARSER_H
#define MYSHELL_MECHO_PARSER_H


#include "builtin_parser.h"

class mecho_parser_t : public builtin_parser_t {
public:
    mecho_parser_t() = default;
    mecho_parser_t(const mecho_parser_t &) = default;
    mecho_parser_t &operator=(const mecho_parser_t &) = delete;
    mecho_parser_t(mecho_parser_t &&) = default;
    mecho_parser_t &operator=(mecho_parser_t &&) = delete;
    ~mecho_parser_t() = default;

    virtual void print_help_message() override;
    virtual void parse(const std::vector<std::string> &av) override;
    const std::vector<std::string> &get_texts() {
        return texts;
    }
protected:
    std::vector<std::string> texts;
};


#endif //MYSHELL_MECHO_PARSER_H
