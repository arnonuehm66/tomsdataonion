#include <stdio.h>
#include <stdlib.h>

#define PARITY_EVEN (0)
#define PARITY_ODD  (!0)

typedef unsigned char uchar;

// Converter struct for 64 bits minus 7 bits parity = 7 bytes.
typedef union u_bb {
  int64_t i64Int;
  uchar   aBytes[8];
} t_bb;


/*******************************************************************************
 * Name:  getParity
 * Purpose: Calculates the bit-parity of the given integer.
 * URL: https://www.geeksforgeeks.org/program-to-find-parity/
 *******************************************************************************/
int getParity(int c) {
  int iParity = 0;

  while (c) {
    iParity = !iParity;
    c       = c & (c - 1);
  }
  // 0: even; !0: odd
  return iParity;
}

/*******************************************************************************
 *
 *******************************************************************************/
void printBytes(uchar* paucByte, int c) {
  t_bb tBb = {0};

  // 8 * 7 bits = 56 bites = 7 Bytes
  // 111 1111  111 1111  111 1111  111 1111  111 1111 111 1111 111 1111 111 1111
  // 1111 1111  1111 1111  1111 1111  1111 1111  1111 1111  1111 1111  1111 1111

  // Compress 8 to 7 bytes.
  for (int i = 0; i < 8; ++i) tBb.i64Int = (tBb.i64Int << 7) | (paucByte[i] >> 1);

  // Print all bytes (watch for little endianess!).
  for (int i = 0; i < 7; ++i) putchar(tBb.aBytes[6 - i]);
}


//******************************************************************************
int main() {
  uchar aucByte[8] = {0};
  int   iCount     =  0;
  int   c          =  0;

  while ((c = getchar()) != EOF) {
    // That's odd parity plus the parity bit itself.
    if (getParity(c) != PARITY_EVEN) continue;

    // Store the byte for further processing and count stored bytes.
    aucByte[iCount++] = c;

    // Store full? Print all bits/bytes.
    if (iCount == 8) {
      printBytes(aucByte, c);
      iCount = 0;
    }
  }
}
