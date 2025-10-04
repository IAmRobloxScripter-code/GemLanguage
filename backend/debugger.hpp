#pragma once
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

inline size_t getTerminalWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    return 80;
#endif
}

template <typename... Args>
inline std::string dynamic_format(std::string_view rt_fmt_str, Args &&...args) {
    return std::vformat(rt_fmt_str, std::make_format_args(args...));
}

enum class error_type {
    lexical_error,
    parsing_error,
    runtime_error,
};

inline std::unordered_map<error_type, std::string> errors{
    {error_type::lexical_error, "Lexical Error: {}\n{}"},
    {error_type::parsing_error, "Parsing Error: {}\n{}"},
    {error_type::runtime_error, "Runtime Error: {}\n{}"},
};

inline std::string error_template = "{}:{} -> {}\n";

inline std::vector<std::string> split_by_width(const std::string& text, size_t width) {
    std::vector<std::string> lines;
    size_t start = 0;

    while (start < text.size()) {
        size_t chunkSize = std::min(width - 2, text.size() - start);
        lines.push_back(text.substr(start, chunkSize));
        start += chunkSize;
    }

    return lines;
}

inline std::string add_pointers(const std::string pointer_character,
    const std::string &message,
    int start,
    int end) {
    std::string pointer_string;
    size_t string_size = message.size();

    for (size_t index = 0; index < string_size; ++index) {
        if (index >= start && index <= end) {
            pointer_string += pointer_character;
        } else {
            pointer_string += " ";
        }
    }

    size_t witdh = getTerminalWidth();

    std::vector<std::string> message_chunks = split_by_width(message, witdh);
    std::vector<std::string> pointer_chunks = split_by_width(pointer_string, witdh);

    std::string finale;

    for (size_t index = 0; index < message_chunks.size(); ++index) {
        finale += "| " + message_chunks[index] + "\n";
        finale += "| " + pointer_chunks[index] + "\n";
    }

    return finale;
}

inline void error(error_type error_type,
    const std::string &message,
    const std::string &file_name,
    uint64_t line,
    std::string error_context = "") {
    std::string error_message = dynamic_format(error_template,
        file_name,
        line,
        dynamic_format(errors[error_type], error_context, message));
    std::cerr << error_message;
}