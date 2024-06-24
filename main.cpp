#include <cstdlib>
#include <iostream>

int main(int argc, char const *argv[]) {
   if (argc != 2) {
      std::cerr << "the number of arguments must be 2, but got " << argc
                << ", Abort." << std::endl;
   }

   const char *p = argv[1];

   auto read_decimal = [&]() {
      return std::strtol(p, const_cast<char **>(&p), 10);
   };

   std::cout << ".intel_syntax noprefix" << "\n";
   std::cout << ".globl main" << "\n";
   std::cout << "main:" << "\n";
   std::cout << "  mov rax, " << read_decimal() << "\n";

   while (*p) {
      if (*p == '+') {
         ++p;
         std::cout << "  add rax, " << read_decimal() << "\n";
         continue;
      }

      if (*p == '-') {
         ++p;
         std::cout << "  sub rax, " << read_decimal() << "\n";
         continue;
      }

      std::cerr << "Unexpected character: " << *p << std::endl;
      return 1;
   }

   std::cout << "  ret" << "\n";
   std::cout << std::flush;

   return 0;
}
