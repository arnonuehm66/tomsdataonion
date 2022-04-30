#include <stdio.h>
#include <stdlib.h>


int processByte(int c) {
  // XOR 0b01010101
  // $b = $b ^ 0x55;
  c = c ^ 0x55;

  // Right bit rotate.
  // $b = $b >> 1 | (($b & 1) << 8);
  c = (c >> 1) | ((c & 1) << 8);

  return c;
}


int main() {
  int c = 0;

  while ((c = getchar()) != EOF) {
    c = processByte(c);
    putchar(c);
  }
}

