#include "cat.h"

#include <iostream>
#include "options_parser.h"

#include <unistd.h> // read
#include <cstdlib> // EXIT_FAILER
#include <fcntl.h> // open

static void write_buf_to_stdout(const char *buf, size_t count) {
    ssize_t num_written;
    while ((num_written = write(STDOUT_FILENO, buf, count))) {
        if (num_written == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << "Error when writing to stdout" << std::endl;
            std::cerr << "errno = " << errno << std::endl;
            exit(EXIT_FAILURE);
        } else {
            count -= num_written;
            buf += num_written;
        }
    }
}

int open_file(const std::string &fname) {
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error when opening file " << fname << std::endl;
        std::cerr << "errno = " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
    return fd;
}

void cat_file(int fd, const std::string &fname, char *buf, size_t buf_size, bool A_flag) {
    ssize_t num_read;
    while ((num_read = read(fd, buf, buf_size))) {
        if (num_read == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << "Error when reading file " << fname << std::endl;
            std::cerr << "errno = " << errno << std::endl;
            exit(EXIT_FAILURE);
        } else {
            if (A_flag) {
                std::string str_buf(buf_size * 4, 'f');
                char * t = str_buf.data();
                for (ssize_t i = 0; i < num_read; ++i) {
                    if (!isprint(buf[i]) && !(isspace(buf[i]))) {
                        sprintf(t, "\\x%02X", static_cast<unsigned char >(buf[i]));
                        t += 4;
                    }
                    else {
                        *t = buf[i];
                        ++t;
                    }
                }
                write_buf_to_stdout(str_buf.data(), t - str_buf.data());
            }
            else {
                write_buf_to_stdout(buf, num_read);
            }
        }
    }
}



