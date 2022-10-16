#ifndef MYCAT_CAT_H
#define MYCAT_CAT_H

#include <cstddef>
#include <string>
#include <cctype>

/**
 * Open a file for reading.
 * @param filename
 * @return descriptor of the opened file
 */
int open_file(const std::string &fname);

/**
 * Cat opened file to stdout
 * @param fd file descriptor of opened file
 * @param fname filename of opened file
 * @param buf pointer to the buffer used to store chunks
 * @param buf_size size of the buffer
 */
void cat_file(int fd, const std::string &fname, char *buf, size_t buf_size, bool A_flag);

#endif //MYCAT_CAT_H
