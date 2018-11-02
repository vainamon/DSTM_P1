#include <stdio.h>

int test(int arg)
{
  printf("llvm_test: Much pretty jit test. Func arg: %d\n",arg);
  return arg;
}

int main()
{
  printf("llvm_test: Hello from JITted main!\n");
  return 500100;
}
