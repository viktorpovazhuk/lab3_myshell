#ifndef MYSHELL_MCD_PARSER_H
#define MYSHELL_MCD_PARSER_H

#include "builtin_parser.h"

class mcd_parser_t : public builtin_parser_t {
public:
    mcd_parser_t() = default;
    mcd_parser_t(const mcd_parser_t &) = default;
    mcd_parser_t &operator=(const mcd_parser_t &) = delete;
    mcd_parser_t(mcd_parser_t &&) = default;
    mcd_parser_t &operator=(mcd_parser_t &&) = delete;
    ~mcd_parser_t() = default;

    virtual void print_help_message() override;
    virtual void parse(const std::vector<std::string> &av) override;

    const std::string &get_path() {
        return path;
    }
protected:
    std::string path;
};


#endif //MYSHELL_MCD_PARSER_H
