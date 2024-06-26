#include <cstdlib>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <list>
#include <sstream>
#include <string_view>

// TODO: hold this other than global variable
std::string_view expr;

void print_error_location_and_exit(std::ostream &os, std::string_view current) {
   const auto pos = current.data() - expr.data();
   if (0 <= pos && static_cast<std::size_t>(pos) <= expr.size()) {
      std::stringstream ss;
      ss << expr << "\n";
      ss << std::setfill(' ') << std::setw(pos) << " ";
      ss << "^";
      os << ss.str();
   }
   std::exit(1);
}

enum class TokenKind {
   Reserved,
   Number,
   EndOfFile,
};
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

using Number = int;

struct Token {
   TokenKind kind;
   std::string_view str;
   Number val;
};

using TokenIter = decltype(std::list<Token>{}.cbegin());

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

auto is_eof(TokenIter &token) -> bool {
   return token->kind == TokenKind::EndOfFile;
}

void debug_tokens(std::ostream &os, const std::list<Token> &tokens) {
   for (const auto &token : tokens) {
      os << "  Token { kind = '" << token.kind << "', str = '" << token.str
         << "' }\n";
   }
   os << std::endl;
}

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
         if (head == "+" || head == "-") {
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

int main(int argc, char const *argv[]) {
   if (argc != 2) {
      std::cerr << "the number of arguments must be 2, but got " << argc
                << ", Abort." << std::endl;
   }

   expr = argv[1];
   const auto tokens = tokenize(expr);
   auto token = tokens.cbegin();

   // DEBUG: debug print tokenized units
   debug_tokens(std::cerr, tokens);

   std::cout << ".intel_syntax noprefix" << "\n";
   std::cout << ".globl main" << "\n";
   std::cout << "main:" << "\n";

   std::cout << "  mov rax, " << expect_number(token) << "\n";
   while (!is_eof(token)) {
      if (consume(token, "+")) {
         std::cout << "  add rax, " << expect_number(token) << "\n";
         continue;
      }

      if (consume(token, "-")) {
         std::cout << "  sub rax, " << expect_number(token) << "\n";
         continue;
      }

      std::cerr << "Unreachable sentence (this is a bug)" << std::endl;
      print_error_location_and_exit(std::cerr, token->str);
   }

   std::cout << "  ret" << "\n";
   std::cout << std::flush;

   return 0;
}
