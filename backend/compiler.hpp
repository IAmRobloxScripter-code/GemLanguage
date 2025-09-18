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
    std::ostringstream out;

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
    void link(const std::string &c_file);
};