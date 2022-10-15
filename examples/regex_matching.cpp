#include <fnmatch.h>
#include <iostream>
#include <string>

int main() {
    std::string pattern = "[xkn]y?.txt";
    int res;

    res = fnmatch(pattern.c_str(), "xyz.txt", FNM_PATHNAME | FNM_PERIOD);
    if (res == 0) {
        std::cout << "Match";
    }
    else if (res != FNM_NOMATCH) {
        perror("Error in matching");
        exit(EXIT_FAILURE);
    }
}

//int main() {
//    std::string pattern = "~/CLionProjects/OS/*/data/*";
//    glob_t globbuf;
//    int res;
//
//    res = glob(pattern.c_str(), GLOB_TILDE_CHECK, nullptr, &globbuf);
//
////    if (res != 0) {
////        perror()
////    }
//
//    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
//        std::cout << globbuf.gl_pathv[i] << '\n';
//    }
//
//    globfree(&globbuf);
//}