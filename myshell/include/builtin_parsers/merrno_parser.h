#ifndef MYSHELL_MERRNO_PARSER_H
#define MYSHELL_MERRNO_PARSER_H

#include "builtin_parser.h"

class merrno_parser_t : public builtin_parser_t {
public:
    merrno_parser_t() = default;
    merrno_parser_t(const merrno_parser_t &) = default;
    merrno_parser_t &operator=(const merrno_parser_t &) = delete;
    merrno_parser_t(merrno_parser_t &&) = default;
    merrno_parser_t &operator=(merrno_parser_t &&) = delete;
    ~merrno_parser_t() = default;

    virtual void print_help_message() override;
};


#endif //MYSHELL_MERRNO_PARSER_H
