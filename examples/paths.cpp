//
// Created by vivi on 15.10.22.
//

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
//    fs::path mypath{".././*"};
//    std::cout << mypath.parent_path().string();

    std::cout << fs::canonical("/proc/self/exe").string() << '\n';
}