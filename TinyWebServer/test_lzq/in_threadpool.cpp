#include <stdio.h>

int add(int a, int b) { return a + b; }

int sum(int (*ad)(int c, int d), int a, int b) {
  int result;
  result = ad(a, b);
  return result;
}

int main() {
  int r = sum(add, 3, 6);
  printf("%d\n", r);
}