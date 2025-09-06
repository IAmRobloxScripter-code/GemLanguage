#include "compiler/compiler.hpp"
#include "compiler/vm.hpp"
#include "gemSettings.hpp"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

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

std::string getBytecode(std::string &src, const std::filesystem::path& file) {
    parser parserInstance;
    astToken ast = parserInstance.produceAST(src);
    compiler compilerInstance;
    std::string bytecode = compilerInstance.compile(ast, file);

    return bytecode;
}

size_t getMemoryUsageKB() {
    std::ifstream statm("/proc/self/status");
    std::string line;
    while (std::getline(statm, line)) {
        if (line.find("VmRSS:") == 0) {
            std::string value = line.substr(6);
            return std::stoul(value);
        }
    }
    return 0;
}

template <typename Func> void benchmarkProgram(Func func) {
    double elapsedSum = 0;
    double memorySum = 0;
    int iterations = 1000;
    for (int i = 0; i < iterations; i++) {
        long long mem_before = static_cast<long long>(getMemoryUsageKB());
        auto start = std::chrono::high_resolution_clock::now();

        func();

        auto end = std::chrono::high_resolution_clock::now();
        long long mem_after = static_cast<long long>(getMemoryUsageKB());

        std::chrono::duration<double, std::milli> elapsed = end - start;
        long long memDelta = mem_after - mem_before;
        if (memDelta < 0)
            memDelta = 0;

        elapsedSum += elapsed.count();
        memorySum += memDelta;
    }

    std::cout << "Average elapsed time: " << (elapsedSum / iterations) << " ms\n";
    std::cout << "Average memory usage: " << (memorySum / iterations) << " KB\n";
}

class flags {
  public:
    void runBytecode(std::string &bytecode) {
        if (settings.benchmark) {
            benchmarkProgram([&]() { evaluate(bytecode); });
        } else {
            evaluate(bytecode);
        }
    }

    void run(std::string &src, const std::filesystem::path& file) {
        std::string bytecode = getBytecode(src, file);
        runBytecode(bytecode);
    }

    void outBytecode(std::string &src, std::string &name, const std::filesystem::path& file) {
        if (settings.benchmark) {
            benchmarkProgram([&]() {
                std::string bytecode = getBytecode(src, file);
                std::ofstream outfile(name);
                outfile << bytecode << std::endl;
                outfile.close();
            });
        } else {
            std::string bytecode = getBytecode(src, file);
            std::ofstream outfile(name);
            outfile << bytecode << std::endl;
            outfile.close();
        }
    }
};

std::string shiftArguments(std::deque<std::string> &src) {
    std::string value = src.front();
    src.pop_front();
    return value;
}

int main(int argc, char *argv[]) {
    try {
        flags flagContainer;
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
        std::string outFile = arguments.empty() ? fileName + ".o" : arguments.front();

        if (outFile == arguments.front() && outFile[0] != '-' && outFile[1] != '-') {
            shiftArguments(arguments);
        } else {
            outFile = fileName + ".o";
        }

        while (!arguments.empty() && arguments.front().size() > 1 && arguments.front()[0] == '-' &&
               arguments.front()[1] == '-') {
            std::string flag = shiftArguments(arguments);
            if (flag == "--verbose") {
                settings.verbose = true;
            } else if (flag == "--optimize") {
                settings.optimize = true;
            } else if (flag == "--benchmark") {
                settings.benchmark = true;
            }
        }

        for (std::string &flag : flags) {
            if (flag == "-b") {
                std::string src = readFile(inputFile);
                flagContainer.runBytecode(src);
            } else if (flag == "-o") {
                std::string src = readFile(inputFile);
                flagContainer.outBytecode(src, outFile, std::filesystem::path(inputFile));
            }
        }

        if (flags.empty()) {
            std::string src = readFile(inputFile);
            flagContainer.run(src, std::filesystem::path(inputFile));
        }
    } catch (const char *msg) {
        std::cerr << "Error: " << msg << "\n";
        return 1;
    }
}