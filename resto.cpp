#include <iostream>

int main(int argc, char** argv) {
   int n, r;
   std::cin >> n;
   r = n % 500;
   std::cout << n << " " << r << std::endl;
   return 0;
}
