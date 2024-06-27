#include <cstdlib>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string_view>

namespace {

using Number = int;

// TODO: hold this other than global variable
std::string_view expr_buffer;
void print_error_location_and_exit(std::ostream &os, std::string_view current);

enum class TokenKind {
   Reserved,
   Number,
   EndOfFile,
};
[[maybe_unused]]
auto operator<<(std::ostream &os, TokenKind kind) -> std::ostream &;

struct Token {
   TokenKind kind;
   std::string_view str;
   Number val;
};
using TokenIter = decltype(std::list<Token>{}.cbegin());

enum class NodeKind {
   Add,      // "+"
   Subtract, // "-"
   Multiply, // "*"
   Divide,   // "/"
   Number,   // integer
};
[[maybe_unused]]
auto operator<<(std::ostream &os, NodeKind kind) -> std::ostream &;

struct Node {
   NodeKind kind;
   std::unique_ptr<Node> lhs;
   std::unique_ptr<Node> rhs;
   Number val;
};

} // namespace

namespace tokenizer {

auto tokenize(std::string_view expr) -> std::list<Token>;

[[maybe_unused]]
void debug_tokens(std::ostream &os, const std::list<Token> &tokens);

} // namespace tokenizer

namespace parser {

auto consume(TokenIter &token, std::string_view str) -> bool;
void expect(TokenIter &token, std::string_view str);
auto expect_number(TokenIter &token) -> Number;
auto is_eof(TokenIter &token) -> bool;

auto make_general_node(NodeKind kind, std::unique_ptr<Node> &&lhs,
                       std::unique_ptr<Node> &&rhs);
auto make_number_node(Number number);

auto primary(TokenIter &token) -> std::unique_ptr<Node>;
auto mul(TokenIter &token) -> std::unique_ptr<Node>;
auto expr(TokenIter &token) -> std::unique_ptr<Node>;

void debug_nodes(std::ostream &os, const std::unique_ptr<Node> &root,
                 std::string_view prefix = " ", std::size_t indent_level = 0,
                 bool has_following_sibling = false);

} // namespace parser

namespace generator {

void gen(std::ostream &os, const std::unique_ptr<Node> &root);

}

int main(int argc, char const *argv[]) {
   if (argc != 2) {
      std::cerr << "the number of arguments must be 2, but got " << argc
                << ", Abort." << std::endl;
   }

   expr_buffer = argv[1];
   const auto tokens = tokenizer::tokenize(expr_buffer);

   // DEBUG: debug print tokenized units
   // tokenizer::debug_tokens(std::cerr, tokens);

   auto token = tokens.cbegin();
   const auto root = parser::expr(token);

   // DEBUG: debug print parsed units
   // parser::debug_nodes(std::cerr, root);

   // prologue
   std::cout << ".intel_syntax noprefix" << "\n";
   std::cout << ".globl main" << "\n";
   std::cout << "main:" << "\n";

   generator::gen(std::cout, root);

   // use stack top as result
   std::cout << "  pop rax" << "\n";
   std::cout << "  ret" << "\n";
   std::cout << std::flush;

   return 0;
}

namespace {

void print_error_location_and_exit(std::ostream &os, std::string_view current) {
   const auto pos = current.data() - expr_buffer.data();
   if (0 <= pos && static_cast<std::size_t>(pos) <= expr_buffer.size()) {
      std::stringstream ss;
      ss << expr_buffer << "\n";
      ss << std::setfill(' ') << std::setw(pos) << " ";
      ss << "^";
      os << ss.str();
   }
   std::exit(1);
}

auto operator<<(std::ostream &os, TokenKind kind) -> std::ostream & {
   std::stringstream ss;
   using enum TokenKind;
   switch (kind) {
#define ENTRY_CASE(name)                                                       \
   case name: {                                                                \
      ss << #name;                                                             \
   } break

      ENTRY_CASE(Reserved);
      ENTRY_CASE(Number);
      ENTRY_CASE(EndOfFile);

#undef ENTRY_CASE
   }
   return os << ss.str();
}

auto operator<<(std::ostream &os, NodeKind kind) -> std::ostream & {
   std::stringstream ss;
   using enum NodeKind;
   switch (kind) {
#define ENTRY_CASE(name)                                                       \
   case name: {                                                                \
      ss << #name;                                                             \
   } break

      ENTRY_CASE(Add);
      ENTRY_CASE(Subtract);
      ENTRY_CASE(Multiply);
      ENTRY_CASE(Divide);
      ENTRY_CASE(Number);

#undef ENTRY_CASE
   }
   return os << ss.str();
}

} // namespace

namespace tokenizer {

auto tokenize(std::string_view expr) -> std::list<Token> {
   std::list<Token> tokens;
   auto remain = expr;
   while (remain.size() > 0) {
      const auto first_char = remain.front();

      const auto split = [&](std::string_view remain, std::size_t pos)
          -> std::pair<std::string_view, std::string_view> {
         const auto head = remain.substr(0, pos);
         const auto tail = remain.substr(pos);
         return {head, tail};
      };

      const auto is_space = [](char c) { return std::isspace(c); };
      if (is_space(first_char)) {
         // just ignore space
         remain = split(remain, 1).second;
         continue;
      }

      {
         const auto [head, tail] = split(remain, 1);
         if (head == "+" || head == "-" || head == "*" || head == "/" ||
             head == "(" || head == ")") {
            tokens.emplace_back(TokenKind::Reserved, head);
            remain = tail;
            continue;
         }
      }

      const auto is_digit = [](char c) { return std::isdigit(c); };
      if (is_digit(first_char)) {
         const char *next_ptr = nullptr;
         const auto val =
             std::strtol(remain.data(), const_cast<char **>(&next_ptr), 10);
         const auto number_length = next_ptr - remain.data();

         const auto [head, tail] = split(remain, number_length);
         tokens.emplace_back(TokenKind::Number, head, val);
         remain = tail;
         continue;
      }

      std::cerr << "Failed to tokenizing" << std::endl;
      print_error_location_and_exit(std::cerr, remain);
   }
   tokens.emplace_back(TokenKind::EndOfFile, remain);

   return tokens;
}

void debug_tokens(std::ostream &os, const std::list<Token> &tokens) {
   for (const auto &token : tokens) {
      os << "  Token { kind = '" << token.kind << "', str = '" << token.str
         << "' }\n";
   }
   os << std::endl;
}

} // namespace tokenizer

namespace parser {

auto consume(TokenIter &token, std::string_view str) -> bool {
   if (token->kind != TokenKind::Reserved || token->str != str) {
      return false;
   }
   ++token;
   return true;
}

void expect(TokenIter &token, std::string_view str) {
   if (!consume(token, str)) {
      std::cerr << "Unexpected token: expected = '" << str << "', actual = '"
                << token->str << "'" << std::endl;
      print_error_location_and_exit(std::cerr, token->str);
   }
}

auto expect_number(TokenIter &token) -> Number {
   if (token->kind != TokenKind::Number) {
      std::cerr << "Unexpected token: expected a number, actual = '"
                << token->str << "'" << std::endl;
      print_error_location_and_exit(std::cerr, token->str);
   }
   const auto val = token->val;
   ++token;
   return val;
}

[[maybe_unused]]
auto is_eof(TokenIter &token) -> bool {
   return token->kind == TokenKind::EndOfFile;
}

auto make_general_node(NodeKind kind, std::unique_ptr<Node> &&lhs,
                       std::unique_ptr<Node> &&rhs) {
   return std::make_unique<Node>(kind, std::move(lhs), std::move(rhs));
}

auto make_number_node(Number number) {
   return std::make_unique<Node>(NodeKind::Number, nullptr, nullptr, number);
}

auto primary(TokenIter &token) -> std::unique_ptr<Node> {
   if (consume(token, "(")) {
      auto node = expr(token);
      expect(token, ")");
      return node;
   }

   return make_number_node(expect_number(token));
}

auto mul(TokenIter &token) -> std::unique_ptr<Node> {
   auto node = primary(token);
   while (true) {
      if (consume(token, "*")) {
         node = make_general_node(NodeKind::Multiply, std::move(node),
                                  primary(token));
      } else if (consume(token, "/")) {
         node = make_general_node(NodeKind::Divide, std::move(node),
                                  primary(token));
      } else {
         return node;
      }
   }
}

auto expr(TokenIter &token) -> std::unique_ptr<Node> {
   auto node = mul(token);
   while (true) {
      if (consume(token, "+")) {
         node = make_general_node(NodeKind::Add, std::move(node), mul(token));
      } else if (consume(token, "-")) {
         node =
             make_general_node(NodeKind::Subtract, std::move(node), mul(token));
      } else {
         return node;
      }
   }
}

void debug_nodes(std::ostream &os, const std::unique_ptr<Node> &root,
                 std::string_view prefix, std::size_t indent_level,
                 bool has_following_sibling) {
   {
      std::stringstream ss;
      ss << prefix;
      if (indent_level > 0) {
         for (auto i = decltype(indent_level){1}; i < indent_level; ++i) {
            ss << " | ";
         }
         if (has_following_sibling) {
            ss << " |-";
         } else {
            ss << " `-";
         }
      }
      ss << " Node { kind = " << root->kind;
      if (root->kind == NodeKind::Number) {
         ss << ", val = " << root->val;
      }
      ss << " }\n";
      os << ss.str();
   }
   if (root->kind != NodeKind::Number) {
      debug_nodes(os, root->lhs, prefix, indent_level + 1, true);
      debug_nodes(os, root->rhs, prefix, indent_level + 1, false);
   }
}

} // namespace parser

namespace generator {

void gen(std::ostream &os, const std::unique_ptr<Node> &root) {
   if (root->kind == NodeKind::Number) {
      std::stringstream ss;
      ss << "  push " << root->val << "\n";
      os << ss.str();
      return;
   }

   gen(os, root->lhs);
   gen(os, root->rhs);

   std::stringstream ss;
   ss << "  pop rdi" << "\n";
   ss << "  pop rax" << "\n";
   using enum NodeKind;
   switch (root->kind) {
   case Add:
      ss << "  add rax, rdi" << "\n";
      break;
   case Subtract:
      ss << "  sub rax, rdi" << "\n";
      break;
   case Multiply:
      ss << "  imul rax, rdi" << "\n";
      break;
   case Divide:
      ss << "  cqo" << "\n";
      ss << "  idiv rdi" << "\n";
      break;
   default:
      // TODO: handle error
      break;
   }

   ss << "  push rax" << "\n";

   os << ss.str();
}

} // namespace generator
