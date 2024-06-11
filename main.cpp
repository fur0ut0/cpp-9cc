#include <cstdlib>
#include <iostream>

int main(int argc, char const *argv[]) {
   if (argc != 2) {
      std::cerr << "the number of arguments must be 2, but got " << argc
                << ", Abort." << std::endl;
   }

   auto number = std::atoi(argv[1]);

   std::cout << ".intel_syntax noprefix" << "\n";
   std::cout << ".globl main" << "\n";
   std::cout << "main:" << "\n";
   std::cout << "  mov rax, " << number << "\n";
   std::cout << "  ret" << "\n";
   std::cout << std::flush;

   return 0;
}
