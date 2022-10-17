#ifndef MYSHELL_MYSHELL_EXCEPTIONS_H
#define MYSHELL_MYSHELL_EXCEPTIONS_H

#include <stdexcept>

class fnmatch_error : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

#endif //MYSHELL_MYSHELL_EXCEPTIONS_H
