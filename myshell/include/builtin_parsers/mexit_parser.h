#ifndef MYSHELL_MEXIT_PARSER_H
#define MYSHELL_MEXIT_PARSER_H

#include <boost/program_options.hpp>

#include "builtin_parser.h"

class mexit_parser_t : public builtin_parser_t {
public:
    mexit_parser_t() = default;
    mexit_parser_t(const mexit_parser_t &) = default;
    mexit_parser_t &operator=(const mexit_parser_t &) = delete;
    mexit_parser_t(mexit_parser_t &&) = default;
    mexit_parser_t &operator=(mexit_parser_t &&) = delete;
    ~mexit_parser_t() = default;

    int get_code() {
        return code;
    }

    virtual void print_help_message() override;
    virtual void parse(const std::vector<std::string> &av) override;

    virtual void setup_description() override;
protected:

    po::positional_options_description pos_options;
    int code;
};


#endif //MYSHELL_MEXIT_PARSER_H
