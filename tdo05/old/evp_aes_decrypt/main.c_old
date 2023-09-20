#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <string.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/evperr.h>


//******************************************************************************

//******************************************************************************
void handleErrors(int iErr) {
//  ERR_print_errors_fp(stderr);
//  abort();
  exit(iErr);
}

//******************************************************************************
int unwrapAesKey(uint8_t* u8pCKey, int iCLen, uint8_t* u8pKey, uint8_t* u8pIV, uint8_t* u8pPKey) {
  EVP_CIPHER_CTX* ctx = NULL;
  int             len1 = 0;
  int             len2 = 0;

  if (! (ctx = EVP_CIPHER_CTX_new())                                    ) handleErrors(1);

  if (! EVP_DecryptInit_ex(ctx, EVP_aes_256_wrap(), NULL, u8pKey, u8pIV)) handleErrors(2);
  if (! EVP_DecryptUpdate(ctx, u8pPKey, &len1, u8pCKey, iCLen)          ) handleErrors(3);
  len2 = len1;
  if (! EVP_DecryptFinal_ex(ctx, u8pCKey + len1, &len1)                 ) handleErrors(4);
  len2 += len1;

  EVP_CIPHER_CTX_free(ctx);

  return len2;
}

//******************************************************************************
int decryptEvpAes(uint8_t* u8pCData, int iCLen, uint8_t* u8pKey, uint8_t* u8pIV, uint8_t* u8pPData) {
  EVP_CIPHER_CTX* ctx  = NULL;
  int             len1 = 0;
  int             len2 = 0;

  /* Create and initialise the context */
  if(! (ctx = EVP_CIPHER_CTX_new())) handleErrors(1);

  /*
  * Initialise the decryption operation. IMPORTANT - ensure you use a key
  * and IV size appropriate for your cipher
  * In this example we are using 256 bit AES (i.e. a 256 bit key). The
  * IV size for *most* modes is the same as the block size. For AES this
  * is 128 bits
  */
  // dataonion v1.0
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, u8pKey, u8pIV) != 1) handleErrors(2);
//  // dataonion v1.1.3
//  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, u8pKey, u8pIV) != 1) handleErrors(2);

  /*
  * Provide the message to be decrypted, and obtain the plaintext output.
  * EVP_DecryptUpdate can be called multiple times if necessary.
  */
  if (EVP_DecryptUpdate(ctx, u8pPData, &len1, u8pCData, iCLen) != 1) handleErrors(3);

  len2 = len1;

  /*
  * Finalise the decryption. Further plaintext bytes may be written at
  * this stage.
  */
  if (EVP_DecryptFinal_ex(ctx, u8pPData + len1, &len1) != 1) handleErrors(4);

  len2 += len1;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return len2;
}
//******************************************************************************


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
  FILE*    hFile        = NULL;
  uint8_t  ui8KEK[32]   = {0};
  uint8_t  ui8KEK_IV[8] = {0};
  uint8_t  ui8EK[40]    = {0};
  uint8_t  ui8EK_IV[16] = {0};
  uint8_t* pui8Data     = NULL;
  uint16_t ui16DataLen  = 0;
  uint8_t* pui8Buff     = NULL;
  AES_KEY  aesKEK       = {0};

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

  // Decrypt data's AES key.
//  unwrapAesKey(ui8EK, 40, ui8KEK, ui8KEK_IV, pui8Buff);
  AES_set_decrypt_key(ui8KEK, 256, &aesKEK);
  AES_unwrap_key(&aesKEK, ui8KEK_IV, pui8Buff, ui8EK, 40);

  // Buffer swap to payload encryption key.
  for (int i =0; i < 40; ++i) ui8EK[i] = pui8Buff[i];

  // Decrypt the payload.
  decryptEvpAes(pui8Data, ui16DataLen, ui8EK, ui8EK_IV, pui8Buff);

  printPayload(pui8Buff, ui16DataLen);

  free(pui8Data);
  free(pui8Buff);

  fclose(hFile);
}
