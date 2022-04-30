//#include <stdio.h>
//#include <stdlib.h>

//// Byte to bits converter struct.
//typedef union ss_byte {
//  unsigned char cByte;
//  struct {
//    // Bits 76543210
//    unsigned char bit0 : 1,
//                  bit1 : 1,
//                  bit2 : 1,
//                  bit3 : 1,
//                  bit4 : 1,
//                  bit5 : 1,
//                  bit6 : 1,
//                  bit7 : 1;
//  };
//} tt_byte;

////******************************************************************************
//int processByte(char* paBits, int c) {

//  if (isOdd(c))
//    add7Bits(paBits);

//  return c;
//}

////******************************************************************************
//void printBytes(char* paBits) {
//  char cByte = 0;

//  for(int i = 0; i < 7; ++i) {

//    cByte = 0;
//    for (int b = 0; b < 8; ++b)
//      cByte = (cByte << 1) & (paBits[i * 7 + b]) ? 1 : 0;

//    printf("%c", cByte);
//  }
//}

////******************************************************************************
//void getByte(char* pcByte, int c) {
//  tt_byte theByte = {0};

//  theByte.cByte = c;

//  pcByte[0] = (theByte.bit7) ? '1' : '0';
//  pcByte[1] = (theByte.bit6) ? '1' : '0';
//  pcByte[2] = (theByte.bit5) ? '1' : '0';
//  pcByte[3] = (theByte.bit4) ? '1' : '0';
//  pcByte[4] = (theByte.bit3) ? '1' : '0';
//  pcByte[5] = (theByte.bit2) ? '1' : '0';
//  pcByte[6] = (theByte.bit1) ? '1' : '0';
//  pcByte[7] = (theByte.bit0) ? '1' : '0';
//  pcByte[8] = 0;
//}

////******************************************************************************
//int main() {
//  char acBits[8 * 7] = {0};
//  int iParity        =  0;
//  while ((c = getchar()) != EOF) {
//    getByte(acByte, c);
//    iParity = getParity(c);
//    printf("% 3i = 0x%02x = 0b%s (%s)\n", c, c, acByte, (iParity == PARITY_ODD) ? "odd" : "even");
//  }
//}

#include <stdio.h>
#include <stdlib.h>

#define PARITY_EVEN (0)
#define PARITY_ODD  (!0)

typedef unsigned char uchar;

// Converter struct for 64 bits minus 7 bits parity = 7 bytes.
typedef union u_bb {
  int64_t i64Int;
  char    aBytes[8];
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
void printBytes(char* pacByte, int c) {
  t_bb tBb = {0};

  // 8 * 7 bits = 56 bites = 7 Bytes
  // 111 1111  111 1111  111 1111  111 1111  111 1111 111 1111 111 1111 111 1111
  // 1111 1111  1111 1111  1111 1111  1111 1111  1111 1111  1111 1111  1111 1111

  // Compress 8 to 7 bytes.
  for (int i = 0; i < 8; ++i) {
    tBb.i64Int = (tBb.i64Int << 7) | (pacByte[i] >> 1);
  }

  // Print all bytes.
  for (int i = 0; i < 7; ++i) {
    putchar(tBb.aBytes[6 - i]);
  }
}


//******************************************************************************
int main() {
  char acByte[8] = {0};
  int  iCount    =  0;
  int  c         =  0;

  while ((c = getchar()) != EOF) {
    // That's odd parity plus the parity bit itself.
    if (getParity(c) != PARITY_EVEN) continue;

    // Store the byte for further processing and count stored bytes.
    acByte[iCount++] = c;

    // Store full? Print all bits/bytes.
    if (iCount == 8) {
      printBytes(acByte, c);
      iCount = 0;
    }
  }
}
