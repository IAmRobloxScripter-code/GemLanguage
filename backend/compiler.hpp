#include "parser.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <fmt/core.h>
#include <fmt/format.h>
#include <filesystem>
#include <optional>

template<typename... Args>
std::string string_format(const std::string &src, Args&&... args) {
    return fmt::vformat(src, fmt::make_format_args(std::forward<Args>(args)...));
}

class gem_compiler {
  public:
    std::vector<std::ostringstream> streams;
    std::ostringstream* out = nullptr;
    std::string file_name;
    std::string compile(astToken &ast);

  public:
    std::optional<std::string> code_gen(astToken &node);
    void code_gen_program(astToken &node);
    void code_gen_number(astToken &node);
    void code_gen_bool(astToken &node);
    void code_gen_string(astToken &node);
    std::string code_gen_var_decl(astToken &node);
    void code_gen_binaryoperation(astToken &node);
    void code_gen_assignmentexpr(astToken &node);
    void code_gen_conditionals(astToken &node);
    void code_gen_ifstmt(astToken &node);
    void code_gen_body(std::vector<std::shared_ptr<astToken>> body);
    void code_gen_forloop(astToken &node);
    void add_header(const std::string &header);
    std::optional<std::string> code_gen_function(astToken &node);
    void link(const std::string &c_file);

  public:
    void make_stream() {
      streams.emplace_back();
      out = &streams.back();
    };

    void free_stream() {
      streams.pop_back();
      out = &streams.back();
    }
};