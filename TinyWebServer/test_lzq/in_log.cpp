#include "stdio.h"
#include <string>

class Log {
private:
  int num = 0;

public:
  int add(int a, int b);
  int reduce(int a, int b);

  int aa;
  int bb;
};

int Log::add(int a, int b) { return a + b; }

int Log::reduce(int a, int b) { return a - b; }

int main() {

  int a = 1;
  int b = 2;
  Log *inst;
  int sum = inst->add(a, b);
  printf("output: %d\n", sum);

  printf("%d, %d\n", 2 / 80000, 2 % 80000);

  std::string s[1];
  std::string ss = "abcdefgh";
  s[0] = ss;
  printf("%s\n", ss.c_str());
  return 0;
}
