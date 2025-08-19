#include "compiler/compiler.hpp"
#include "compiler/vm.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

std::string readFile(const std::string &path)
{
    std::ifstream file;
    file.open(path);

    if (!file.is_open())
    {
        std::cerr << "Error opening the file!";
        return "";
    }

    std::string s;
    std::string src;

    while (getline(file, s))
    {
        src += s + '\n';
    };

    file.close();

    return src;
}

std::string getBytecode(std::string &src)
{
    parser parserInstance;
    astToken ast = parserInstance.produceAST(src);
    compiler compilerInstance;
    std::string bytecode = compilerInstance.compile(ast);

    return bytecode;
}

class flags
{
public:
    void runBytecode(std::string &bytecode)
    {
        evaluate(bytecode);
    }

    void run(std::string &src)
    {
        std::string bytecode = getBytecode(src);
        runBytecode(bytecode);
    }

    void outBytecode(std::string &src, std::string &name)
    {
        std::ofstream outfile(name);
        std::string bytecode = getBytecode(src);
        outfile << bytecode << std::endl;
        outfile.close();
    }
};

int main(int argc, char *argv[])
{
    flags flagContainer;
    std::string inputFile = argc > 1 ? argv[1] : "";
    std::string fileName = std::filesystem::path(inputFile).stem();

    if (inputFile == "")
    {
        std::cout << "no file provided --terminated" << std::endl;
        exit(0);
    }

    if (!std::filesystem::exists(inputFile))
    {
        std::cout << "file does not exist --terminated" << std::endl;
        exit(0);
    }

    std::string flag = argc > 2 ? argv[2] : "";

    if (flag == "-o")
    {
        std::string outFile = argc > 3 ? argv[3] : fileName;
        std::string src = readFile(inputFile);
        flagContainer.outBytecode(src, outFile);
    }
    else if (flag == "-b")
    {
        std::string bytecode = readFile(inputFile);
        flagContainer.runBytecode(bytecode);
    }
    else if (flag == "")
    {
        std::string src = readFile(inputFile);
        flagContainer.run(src);
    }
    else
    {
        std::cout << "invalid flag --terminated" << std::endl;
        exit(0);
    }

    return 0;
}