// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vector>
#include "options_parser.h"

#include <unistd.h> // read

#include <cat.h>

const size_t CHUNK_SIZE = 16384;

int main(int argc, char* argv[]) {
    command_line_options_t command_line_options{argc, argv};
    const std::vector<std::string> fnames = command_line_options.get_filenames();
    bool A_flag = command_line_options.get_A_flag();

    std::string str_buf(CHUNK_SIZE, 'a');

    std::vector<int> fds;
    fds.reserve(fnames.size());

    for (const std::string &fname : fnames) {
        int fd = open_file(fname);
        fds.push_back(fd);
    }
    for (size_t idx = 0; idx < fds.size(); ++idx) {
        cat_file(fds[idx], fnames[idx], str_buf.data(), str_buf.size(), A_flag);
    }
    return 0;
}
