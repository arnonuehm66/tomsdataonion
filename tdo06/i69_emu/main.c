#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define REG8_A 0
#define REG8_B 1
#define REG8_C 2
#define REG8_D 3
#define REG8_E 4
#define REG8_F 5

#define REG32_LA  0
#define REG32_LB  1
#define REG32_LC  2
#define REG32_LD  3
#define REG32_PTR 4
#define REG32_PC  5


typedef struct s_cpu {
  uint8_t  reg8[7];
  uint32_t reg32[6];
} t_cpu;

t_cpu g_i69;


//******************************************************************************
long int getSizeOfFile(FILE* hFile) {
  long int liSize = 0;

  fseek(hFile, 0, SEEK_END);
    liSize = ftell(hFile);
  fseek(hFile, 0, SEEK_SET);

  return liSize;
}

//******************************************************************************
//* 1 element = OK, 0 elements = EOF
int readBytes(void* pvBytes, int iLength, FILE* hFile) {
  size_t sBytesRead = 0;
  sBytesRead = fread(pvBytes, iLength, 1, hFile);
  return sBytesRead;
}

//******************************************************************************
void printPayload(uint8_t* pucBytes, int iLength) {
  for (int i = 0; i < iLength; ++i)
    printf("%c", pucBytes[i]);
}

//******************************************************************************
void initCpu(void) {
  for (int i = 0; i < 7; ++i) g_i69.reg8[i]  = 0;
  for (int i = 0; i < 6; ++i) g_i69.reg32[i] = 0;
}

//******************************************************************************
uint8_t get_instruction(uint8_t* memory) {
  uint8_t instruction = memory[g_i69.reg32[REG32_PC]];
  ++g_i69.reg32[REG32_PC];
  return instruction;
}

//******************************************************************************
uint8_t get_imm8(uint8_t* memory) {
  uint8_t imm8 = memory[g_i69.reg32[REG32_PC]];
  ++g_i69.reg32[REG32_PC];
  return imm8;
}

//******************************************************************************
uint32_t get_imm32(uint8_t* memory) {
  uint32_t imm32 = *((uint32_t*) (&memory[g_i69.reg32[REG32_PC]]));
  g_i69.reg32[REG32_PC] += 4;
  return imm32;
}

//******************************************************************************
void ddd_sss8(uint8_t mv_ddd, uint8_t mv_sss, uint8_t* memory) {
  uint32_t ptr_c = g_i69.reg32[REG32_PTR] + g_i69.reg8[REG8_C];

  if (mv_ddd <  7 && mv_sss <  7)
    g_i69.reg8[mv_ddd - 1] = g_i69.reg8[mv_sss - 1];

  if (mv_ddd <  7 && mv_sss == 7)
    g_i69.reg8[mv_ddd - 1] = memory[ptr_c];

  if (mv_ddd == 7 && mv_sss <  7)
    memory[ptr_c] = g_i69.reg8[mv_sss - 1];
}

//******************************************************************************
void ddd_imm8(uint8_t mv_ddd, uint8_t imm8, uint8_t* memory) {
  uint32_t ptr_c = g_i69.reg32[REG32_PTR] + g_i69.reg8[REG8_C];

  if (mv_ddd <  7)
    g_i69.reg8[mv_ddd - 1] = imm8;

  if (mv_ddd == 7)
    memory[ptr_c] = imm8;
}

//******************************************************************************
void ddd_sss32(uint8_t mv_ddd, uint8_t mv_sss) {
  g_i69.reg32[mv_ddd - 1] = g_i69.reg32[mv_sss - 1];
}

//******************************************************************************
void ddd_imm32(uint8_t mv_ddd, uint32_t imm32) {
  g_i69.reg32[mv_ddd - 1] = imm32;
}

//******************************************************************************
int execute(uint8_t* memory) {
  uint8_t  imm8        = 0;
  uint32_t imm32       = 0;
  uint8_t  mv_ins      = 0;
  uint8_t  mv_ddd      = 0;
  uint8_t  mv_sss      = 0;
  uint8_t  instruction = get_instruction(memory);

  switch(instruction) {
    // ADD a <- b
    case 0xC2 :
      g_i69.reg8[REG8_A] += g_i69.reg8[REG8_B];
      return 1;

    // APTR imm8
    case 0xE1 :
      imm8                    = get_imm8(memory);
      g_i69.reg32[REG32_PTR] += imm8;
      return 1;

    // CMP
    case 0xC1 :
      g_i69.reg8[REG8_F] = (g_i69.reg8[REG8_A] == g_i69.reg8[REG8_B]) ? 0x00 : 0x01;
      return 1;

    // HALT
    case 0x01 :
      return 0;

    // JEZ imm32
    case 0x21 :
      imm32 = get_imm32(memory);
      if (g_i69.reg8[REG8_F] == 0x00)
        g_i69.reg32[REG32_PC] = imm32;
      return 1;

    // JNZ imm32
    case 0x22 :
      imm32 = get_imm32(memory);
      if (g_i69.reg8[REG8_F] != 0x00)
        g_i69.reg32[REG32_PC] = imm32;
      return 1;

    // OUT a
    case 0x02 :
      printf("%c", g_i69.reg8[REG8_A]);
      return 1;

    // SUB a <- b
    case 0xC3 :
      g_i69.reg8[REG8_A] -= g_i69.reg8[REG8_B];
      return 1;

    // XOR a <- b
    case 0xC4 :
      g_i69.reg8[REG8_A] ^= g_i69.reg8[REG8_B];
      return 1;

    // Else is a MV* instruction or an error state.
    default :
      // 0b 01DD DSSS 0x 4 0
      // 0b 10DD DSSS 0x 8 0
      // --------------------
      // 0b 1100 0000 0x c 0
      // 0b 0011 1000 0x 3 8
      // 0b 0000 0111 0x 0 7
      mv_ins =  instruction & 0xc0;
      mv_ddd = (instruction & 0x38) >> 3;
      mv_sss =  instruction & 0x07;

      // The error states!
      if ( mv_ddd == 0)                                   return 0;
      if ( mv_ddd == 7 && mv_sss == 7)                    return 0;
      if ((mv_ddd >  7 || mv_sss >  7) && mv_ins == 0x40) return 0;
      if ((mv_ddd >  6 || mv_sss >  6) && mv_ins == 0x80) return 0;

      // MV
      if (mv_ins == 0x40 && mv_sss > 0) {
        ddd_sss8(mv_ddd, mv_sss, memory);
        return 1;
      }

      // MVI
      if (mv_ins == 0x40 && mv_sss == 0) {
        imm8 = get_imm8(memory);
        ddd_imm8(mv_ddd, imm8, memory);
        return 1;
      }

      // MV32
      if (mv_ins == 0x80 && mv_sss > 0) {
        ddd_sss32(mv_ddd, mv_sss);
        return 1;
      }

      // MVI32
      if (mv_ins == 0x80 && mv_sss == 0) {
        imm32 = get_imm32(memory);
        ddd_imm32(mv_ddd, imm32);
        return 1;
      }

      // Non valid MV instruction.
      return 0;
  }
}


//******************************************************************************
int main(int argc, char* argv[]) {
  FILE*    hFile  = NULL;
  uint8_t* memory = NULL;
  long int memlen = 0;

  if (! (hFile = fopen(argv[1], "rb"))) {
    perror("couldn't open file");
    return 1;
  }

  memlen = getSizeOfFile(hFile);
  memory = (uint8_t*) malloc(memlen);

  // Get the i69 code.
  if (! readBytes(memory, memlen, hFile)) return 1;

  initCpu();

  // Run until an error state or an HALT instruction occurr.
  while (execute(memory)) ;

  free(memory);

  fclose(hFile);
}
