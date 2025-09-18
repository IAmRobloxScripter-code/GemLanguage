#include "gemSettings.hpp"
#include "./backend/compiler.hpp"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// GGC - GENERAL GEM COMPILER

std::string readFile(const std::string &path) {
    if (settings.verbose)
        std::cout << "Reading file: " + path << std::endl;

    std::ifstream file;
    file.open(path);

    if (!file.is_open()) {
        std::cerr << "Error opening the file!";
        return "";
    }

    std::string s;
    std::string src;

    while (getline(file, s)) {
        src += s + '\n';
    };

    file.close();

    if (settings.verbose)
        std::cout << "File has been read successfully" << std::endl;

    return src;
}

std::string shiftArguments(std::deque<std::string> &src) {
    std::string value = src.front();
    src.pop_front();
    return value;
}

void outfile(std::string &src, std::string &outname) {
    std::ofstream file(outname);

    if (file.is_open()) {
        parser parser_instance;
        auto ast = parser_instance.produceAST(src);
        auto compiler = new gem_compiler;

        std::string gemC = compiler->compile(ast);
        file << gemC;
        file.close();

        delete compiler;
    }
}

int main(int argc, char *argv[]) {
    try {
        std::string inputFile = argc > 1 ? argv[1] : "";
        std::string fileName = std::filesystem::path(inputFile).stem();
        std::deque<std::string> arguments;

        for (int i = 0; i < argc; ++i) {
            arguments.push_back(std::string(argv[i]));
        }
        shiftArguments(arguments);

        // flags
        std::vector<std::string> flags;
        if (inputFile != "") {
            shiftArguments(arguments);
        }
        while (!arguments.empty() && arguments.front().size() > 1 && arguments.front()[0] == '-' &&
               arguments.front()[1] != '-') {
            std::string flag = shiftArguments(arguments);

            if (std::find(flags.begin(), flags.end(), flag) != flags.end()) {
                std::cerr << "Error: Cannot use the same flag twice\n";
                return 0;
            }

            flags.push_back(flag);
        }
        std::string outFile = arguments.empty() ? fileName + ".c" : arguments.front();

        if (outFile == arguments.front() && outFile[0] != '-' && outFile[1] != '-') {
            shiftArguments(arguments);
        } else {
            outFile = fileName + ".c";
        }

        while (!arguments.empty() && arguments.front().size() > 1 && arguments.front()[0] == '-' &&
               arguments.front()[1] == '-') {
            std::string flag = shiftArguments(arguments);
            if (flag == "--verbose") {
                settings.verbose = true;
            }
        }

        for (std::string &flag : flags) {
            if (flag == "-o") {
                std::string src = readFile(inputFile);
                outfile(src, outFile);
            } else if (flag == "-d") {
                settings.debug = true;
            }
        }

        if (flags.empty()) {
            std::string src = readFile(inputFile);
        }
    } catch (const char *msg) {
        std::cerr << "Error: " << msg << "\n";
        return 1;
    }
}