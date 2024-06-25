#include <cstdlib>
#include <iosfwd>
#include <iostream>
#include <list>
#include <sstream>
#include <string_view>

enum class TokenKind {
   Reserved,
   Number,
   EndOfFile,
};
auto operator<<(std::ostream &os, TokenKind kind) -> std::ostream & {
   std::stringstream ss;
   using enum TokenKind;
   switch (kind) {
   case Reserved:
      ss << "Reserved";
      break;
   case Number:
      ss << "Number";
      break;
   case EndOfFile:
      ss << "EndOfFile";
      break;
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
      std::exit(1);
   }
}

auto expect_number(TokenIter &token) -> Number {
   if (token->kind != TokenKind::Number) {
      std::cerr << "Unexpected token: expected a number, actual = '"
                << token->str << "'" << std::endl;
      std::exit(1);
   }
   const auto val = token->val;
   ++token;
   return val;
}

auto is_eof(TokenIter &token) -> bool {
   return token->kind == TokenKind::EndOfFile;
}

auto debug_tokens(std::ostream &os, const std::list<Token> &tokens) {
   for (const auto &token : tokens) {
      os << "  Token { kind = '" << token.kind << "', str = '" << token.str
         << "' }\n";
   }
   os << std::endl;
}

auto tokenize(std::string_view expr) -> std::list<Token> {
   std::list<Token> tokens;
   while (expr.size() > 0) {
      const auto first_char = expr.front();

      const auto split = [&](std::string_view expr, std::size_t pos)
          -> std::pair<std::string_view, std::string_view> {
         const auto head = expr.substr(0, pos);
         const auto tail = expr.substr(pos);
         return {head, tail};
      };

      const auto is_space = [](char c) { return std::isspace(c); };
      if (is_space(first_char)) {
         // just ignore space
         expr = split(expr, 1).second;
         continue;
      }

      {
         const auto [head, tail] = split(expr, 1);
         if (head == "+" || head == "-") {
            tokens.emplace_back(TokenKind::Reserved, head);
            expr = tail;
            continue;
         }
      }

      const auto is_digit = [](char c) { return std::isdigit(c); };
      if (is_digit(first_char)) {
         const char *next_ptr = nullptr;
         const auto val =
             std::strtol(expr.data(), const_cast<char **>(&next_ptr), 10);
         const auto number_length = next_ptr - expr.data();

         const auto [head, tail] = split(expr, number_length);
         tokens.emplace_back(TokenKind::Number, head, val);
         expr = tail;
         continue;
      }

      std::cerr << "Failed to tokenizing" << std::endl;
      std::exit(1);
   }
   tokens.emplace_back(TokenKind::EndOfFile, expr);

   return tokens;
}

int main(int argc, char const *argv[]) {
   if (argc != 2) {
      std::cerr << "the number of arguments must be 2, but got " << argc
                << ", Abort." << std::endl;
   }

   const auto tokens = tokenize(argv[1]);
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
      std::exit(1);
   }

   std::cout << "  ret" << "\n";
   std::cout << std::flush;

   return 0;
}
