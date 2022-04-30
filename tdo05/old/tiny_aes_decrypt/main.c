#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <openssl/aes.h>

#include "aes_hc.h"


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
int main(int argc, char* argv[]) {
  FILE*     hFile        = NULL;
  uint8_t   ui8KEK[32]   = {0};
  uint8_t   ui8KEK_IV[8] = {0};
  uint8_t   ui8EK[40]    = {0};
  uint8_t   ui8EK_IV[16] = {0};
  uint8_t*  pui8Data     = NULL;
  uint16_t  ui16DataLen  = 0;
  uint8_t*  pui8Buff     = NULL;
  AES_KEY   aesKEK       = {0};
  t_AES_ctx aesEK        = {0};

  if (! (hFile = fopen(argv[1], "rb"))) {
    perror("couldn't open file");
    return 1;
  }

  ui16DataLen = getSizeOfFile(hFile) - 32 - 8 - 40 - 16;
  pui8Data    = (uint8_t*) malloc(ui16DataLen);
  pui8Buff    = (uint8_t*) malloc(ui16DataLen);

  // First 32 bytes: The 256-bit key encrypting key (KEK).
  if (! readBytes(ui8KEK, 32, hFile)) return 1;
  // Next 8 bytes: The 64-bit initialization vector (IV) for the wrapped key.
  if (! readBytes(ui8KEK_IV, 8, hFile)) return 2;

  // Next 40 bytes: The wrapped (encrypted) key. When decrypted, this will become the 256-bit encryption key.
  if (! readBytes(ui8EK, 40, hFile)) return 3;
  // Next 16 bytes: The 128-bit initialization vector (IV) for the encrypted payload.
  if (! readBytes(ui8EK_IV, 16, hFile)) return 4;

  // All remaining bytes: The encrypted payload.
  if (! readBytes(pui8Data, ui16DataLen, hFile)) return 5;

  // openssl: Decrypt data's AES key.
  AES_set_decrypt_key(ui8KEK, 256, &aesKEK);
  AES_unwrap_key(&aesKEK, ui8KEK_IV, pui8Buff, ui8EK, 40);

  // Buffer swap to payload encryption key.
  for (int i = 0; i < 40; ++i) ui8EK[i] = pui8Buff[i];

  // Decrypt the payload.

//  // tiny-aes way: tomsdataonion v1.1
//  AES_init_ctx_iv(&aesEK, ui8EK, ui8EK_IV);
//  AES_CBC_decrypt_buffer(&aesEK, pui8Data, ui16DataLen);

  // tiny-aes way: tomsdataonion v1.1.3
  AES_init_ctx_iv(&aesEK, ui8EK, ui8EK_IV);
  AES_CTR_xcrypt_buffer(&aesEK, pui8Data, ui16DataLen);

  printPayload(pui8Data, ui16DataLen);

  free(pui8Data);
  free(pui8Buff);

  fclose(hFile);
}
