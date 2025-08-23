#include "compiler/compiler.hpp"
#include "compiler/vm.hpp"
#include "gemSettings.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <deque>

// GGC - GENERAL GEM COMPILER

std::string readFile(const std::string &path)
{
    if (settings.verbose)
        std::cout << "Reading file: " + path << std::endl;

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

    if (settings.verbose)
        std::cout << "File has been read successfully" << std::endl;

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

/*
int main(int argc, char *argv[])
{
    flags flagContainer;
    std::string inputFile = argc > 1 ? argv[1] : "";
    std::string fileName = std::filesystem::path(inputFile).stem();

    if (inputFile == "")
    {
        throw "no file provided --terminated";
    }

    if (!std::filesystem::exists(inputFile))
    {
        throw "file does not exist --terminated";
    }

    std::string flag = argc > 2 ? argv[2] : "";

    for (int i = 4; i < argc; i++)
    {
    }

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
        throw "invalid flag --terminated";
    }

    return 0;
}
*/

std::string shiftArguments(std::deque<std::string> &src)
{
    std::string value = src.front();
    src.pop_front();
    return value;
}

int main(int argc, char *argv[])
{
    try
    {
        flags flagContainer;
        std::string inputFile = argc > 1 ? argv[1] : "";
        std::string fileName = std::filesystem::path(inputFile).stem();
        std::deque<std::string> arguments;

        for (int i = 0; i < argc; ++i)
        {
            arguments.push_back(std::string(argv[i]));
        }
        shiftArguments(arguments);

        // flags
        std::vector<std::string> flags;
        if (inputFile != "")
        {
            shiftArguments(arguments);
        }
        while (!arguments.empty() && arguments.front().size() > 1 && arguments.front()[0] == '-' && arguments.front()[1] != '-')
        {
            std::string flag = shiftArguments(arguments);

            if (std::find(flags.begin(), flags.end(), flag) != flags.end())
            {
                std::cerr << "Error: Cannot use the same flag twice\n";
                return 0;
            }

            flags.push_back(flag);
        }
        std::string outFile = arguments.empty() ? fileName + ".o" : arguments.front();

        if (outFile == arguments.front() && outFile[0] != '-' && outFile[1] != '-')
        {
            shiftArguments(arguments);
        }
        else
        {
            outFile = fileName + ".o";
        }

        while (!arguments.empty() && arguments.front().size() > 1 && arguments.front()[0] == '-' && arguments.front()[1] == '-')
        {
            std::string flag = shiftArguments(arguments);
            if (flag == "--verbose")
            {
                settings.verbose = true;
            }
            else if (flag == "--optimize")
            {
                settings.optimize = true;
            }
        }

        for (std::string &flag : flags)
        {
            if (flag == "-b")
            {
                std::string src = readFile(inputFile);
                flagContainer.runBytecode(src);
            }
            else if (flag == "-o")
            {
                std::string src = readFile(inputFile);
                flagContainer.outBytecode(src, outFile);
            }
        }

        if (flags.empty())
        {
            std::string src = readFile(inputFile);
            flagContainer.run(src);
        }
    }
    catch (const char* msg)
    {
        std::cerr << "Error: " << msg << "\n";
        return 1;
    }
}