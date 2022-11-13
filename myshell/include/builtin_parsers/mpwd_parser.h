#ifndef MYSHELL_MPWD_PARSER_H
#define MYSHELL_MPWD_PARSER_H

#include "builtin_parser.h"

class mpwd_parser_t : public builtin_parser_t {
public:
    mpwd_parser_t() = default;
    mpwd_parser_t(const mpwd_parser_t &) = default;
    mpwd_parser_t &operator=(const mpwd_parser_t &) = delete;
    mpwd_parser_t(mpwd_parser_t &&) = default;
    mpwd_parser_t &operator=(mpwd_parser_t &&) = delete;
    ~mpwd_parser_t() = default;

    virtual void print_help_message() override;
};


#endif //MYSHELL_MPWD_PARSER_H
