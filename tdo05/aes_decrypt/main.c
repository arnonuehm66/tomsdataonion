/*******************************************************************************
 ** Name: aes_decrypt
 ** Purpose: Decrypts wrapped aes key and ctr / cbc crypted data.
 ** Author: (JE) Jens Elstner <jens.elstner@bka.bund.de>
 *******************************************************************************
 ** Date        User  Log
 **-----------------------------------------------------------------------------
 ** 26.08.2020  JE    Created program.
 ** 07.09.2020  JE    Changed it with the standard program skeleton.
 ** 07.09.2020  JE    Now use stdfcns.c v0.8.1 with readBytes(), printBytes().
 ** 13.08.2023  JE    Now uses 'c_dynamic_arrays_macros.h' and latest libs.
 *******************************************************************************/


//******************************************************************************
//* includes & namespaces

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/evperr.h>

#include "c_string.h"
#include "c_dynamic_arrays_macros.h"


//******************************************************************************
//* me and myself

#define ME_VERSION "0.2.3"
cstr g_csMename;


//******************************************************************************
//* defines & macros

#define ERR_NOERR 0x00
#define ERR_ARGS  0x01
#define ERR_FILE  0x02
#define ERR_CRYPT 0x03
#define ERR_ELSE  0xff

#define sERR_ARGS  "Argument error"
#define sERR_FILE  "File error"
#define sERR_CRYPT "Crypto error"
#define sERR_ELSE  "Unknown error"


//******************************************************************************
//* outsourced standard functions, includes and defines

#include "stdfcns.c"


//******************************************************************************
//* typedefs

// Arguments and options.
typedef struct s_options {
  int iUseCtr;
} t_options;

s_array(cstr);


//******************************************************************************
//* Global variables

// Arguments
t_options     g_tOpts;  // CLI options and arguments.
t_array(cstr) g_tArgs;  // Free arguments.


//******************************************************************************
//* Functions

/*******************************************************************************
 * Name:  usage
 * Purpose: Print help text and exit program.
 *******************************************************************************/
void usage(int iErr, const char* pcMsg) {
  cstr csMsg = csNew(pcMsg);

  // Print at least one newline with message.
  if (csMsg.len != 0)
    csCat(&csMsg, csMsg.cStr, "\n\n");

  csSetf(&csMsg, "%s"
//|************************ 80 chars width ****************************************|
  "usage: %s [-r] file1 [file2 ...]\n"
  "       %s [-h|--help|-v|--version]\n"
  " Decrypts wrapped aes key and ctr / cbc crypted data.\n"
  "  -r:            drcrypt with ctr 256 (default cbc 256)\n"
  "  -h|--help:     print this help\n"
  "  -v|--version:  print version of program\n"
//|************************ 80 chars width ****************************************|
         ,csMsg.cStr,
         g_csMename.cStr, g_csMename.cStr
        );

  if (iErr == ERR_NOERR)
    printf("%s", csMsg.cStr);
  else
    fprintf(stderr, "%s", csMsg.cStr);

  csFree(&csMsg);

  exit(iErr);
}

/*******************************************************************************
 * Name:  dispatchError
 * Purpose: Print out specific error message, if any occurres.
 *******************************************************************************/
void dispatchError(int rv, const char* pcMsg) {
  cstr csMsg = csNew(pcMsg);
  cstr csErr = csNew("");

  if (rv == ERR_NOERR) return;

  if (rv == ERR_ARGS)  csSet(&csErr, sERR_ARGS);
  if (rv == ERR_FILE)  csSet(&csErr, sERR_FILE);
  if (rv == ERR_CRYPT) csSet(&csErr, sERR_CRYPT);
  if (rv == ERR_ELSE)  csSet(&csErr, sERR_ELSE);

  // Set to '<err>: <message>', if a message was given.
  if (csMsg.len != 0) csSetf(&csErr, "%s: %s", csErr.cStr, csMsg.cStr);

  usage(rv, csErr.cStr);
}

/*******************************************************************************
 * Name:  getOptions
 * Purpose: Filters command line.
 *******************************************************************************/
void getOptions(int argc, char* argv[]) {
  cstr csArgv = csNew("");
  cstr csRv   = csNew("");
  cstr csOpt  = csNew("");
  int  iArg   = 1;  // Omit program name in arg loop.
  int  iChar  = 0;
  char cOpt   = 0;

  // Set defaults.
  g_tOpts.iUseCtr = 0;

  // Init free argument's dynamic array.
  daInit(cstr, g_tArgs);

  // Loop all arguments from command line POSIX style.
  while (iArg < argc) {
next_argument:
    shift(&csArgv, &iArg, argc, argv);
    if(strcmp(csArgv.cStr, "") == 0)
      continue;

    // Long options:
    if (csArgv.cStr[0] == '-' && csArgv.cStr[1] == '-') {
      if (!strcmp(csArgv.cStr, "--help")) {
        usage(ERR_NOERR, "");
      }
      if (!strcmp(csArgv.cStr, "--version")) {
        version();
      }
      dispatchError(ERR_ARGS, "Invalid long option");
    }

    // Short options:
    if (csArgv.cStr[0] == '-') {
      for (iChar = 1; iChar < csArgv.len; ++iChar) {
        cOpt = csArgv.cStr[iChar];
        if (cOpt == 'h') {
          usage(ERR_NOERR, "");
        }
        if (cOpt == 'v') {
          version();
        }
        if (cOpt == 'r') {
          g_tOpts.iUseCtr = 1;
          continue;
        }
        dispatchError(ERR_ARGS, "Invalid short option");
      }
      goto next_argument;
    }
    // Else, it's just a filename.
    daAdd(cstr, g_tArgs, csNew(csArgv.cStr));
  }

  // Sanity check of arguments and flags.
  if (g_tArgs.sCount == 0) dispatchError(ERR_ARGS, "No file given");

  // Free string memory.
  csFree(&csArgv);
  csFree(&csRv);
  csFree(&csOpt);
}

/*******************************************************************************
 * Name:  decryptEvpAes
 * Purpose: Decrypts given data with given key and IV.
 * ToDo:  Change openssl interface. This one is deprecated!
 *******************************************************************************/
int decryptEvpAes(uint8_t* u8pCData, int iCLen, uint8_t* u8pKey, uint8_t* u8pIV, uint8_t* u8pPData) {
  EVP_CIPHER_CTX* ctx  = NULL;
  int             len1 = 0;
  int             len2 = 0;

  // Create and initialise the context
  if(! (ctx = EVP_CIPHER_CTX_new()))
    dispatchError(ERR_CRYPT, "EVP_CIPHER_CTX_new() failed");

  // Initialise the decryption operation. IMPORTANT - ensure you use a key
  // and IV size appropriate for your cipher
  if (g_tOpts.iUseCtr) {
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, u8pKey, u8pIV) != 1)
      dispatchError(ERR_CRYPT, "CTR: EVP_DecryptInit_ex() failed");
  }
  else {
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, u8pKey, u8pIV) != 1)
      dispatchError(ERR_CRYPT, "CBC: EVP_DecryptInit_ex() failed");
  }

  // EVP_DecryptUpdate can be called multiple times if necessary.
  if (EVP_DecryptUpdate(ctx, u8pPData, &len1, u8pCData, iCLen) != 1)
    dispatchError(ERR_CRYPT, "EVP_DecryptUpdate() failed");

  len2 = len1;

  // Finalise the decryption. Further plaintext bytes may be written at
  // this stage.
  if (EVP_DecryptFinal_ex(ctx, u8pPData + len1, &len1) != 1)
    dispatchError(ERR_CRYPT, "EVP_DecryptFinal_ex() failed");

  len2 += len1;

  // Clean up
  EVP_CIPHER_CTX_free(ctx);

  return len2;
}

/*******************************************************************************
 * Name:  decryptKeyAndFile
 * Purpose: Main function to retrieve all assets and decrypt the data.
 *******************************************************************************/
void decryptKeyAndFile(FILE* hFile, li liFileSize) {
  uint8_t  ui8KEK[32]   = {0};
  uint8_t  ui8KEK_IV[8] = {0};
  AES_KEY  aesKEK       = {0};
  uint8_t  ui8EK[40]    = {0};
  uint8_t  ui8EK_IV[16] = {0};
  uint8_t* pui8Data     = NULL;
  uint16_t ui16DataLen  = 0;
  uint8_t* pui8Buff     = NULL;

  ui16DataLen = liFileSize - 32 - 8 - 40 - 16;
  pui8Data    = (uint8_t*) malloc(ui16DataLen);
  pui8Buff    = (uint8_t*) malloc(ui16DataLen);

  // First 32 bytes: The 256-bit key encrypting key (KEK).
  if (! readBytes(ui8KEK, 32, hFile))
    dispatchError(ERR_FILE, "Couldn't read KEK");

  // Next 8 bytes: The 64-bit initialization vector (IV) for the wrapped key.
  if (! readBytes(ui8KEK_IV, 8, hFile))
    dispatchError(ERR_FILE, "Couldn't read KEK IV");

  // Next 40 bytes: The wrapped (encrypted) key. When decrypted, this will become the 256-bit encryption key.
  if (! readBytes(ui8EK, 40, hFile))
    dispatchError(ERR_FILE, "Couldn't read EK");

  // Next 16 bytes: The 128-bit initialization vector (IV) for the encrypted payload.
  if (! readBytes(ui8EK_IV, 16, hFile))
    dispatchError(ERR_FILE, "Couldn't read EK IV");

  // All remaining bytes: The encrypted payload.
  if (! readBytes(pui8Data, ui16DataLen, hFile))
    dispatchError(ERR_FILE, "Couldn't read data");

  // Decrypt data's AES key.
  AES_set_decrypt_key(ui8KEK, 256, &aesKEK);
  AES_unwrap_key(&aesKEK, ui8KEK_IV, pui8Buff, ui8EK, 40);

  // Buffer swap to payload encryption key.
  for (int i =0; i < 40; ++i) ui8EK[i] = pui8Buff[i];

  // Decrypt the payload.
  decryptEvpAes(pui8Data, ui16DataLen, ui8EK, ui8EK_IV, pui8Buff);

  printBytes(pui8Buff, ui16DataLen);

  free(pui8Data);
  free(pui8Buff);
}


//******************************************************************************
//* main

int main(int argc, char *argv[]) {
  FILE* hFile  = NULL;
  li    liSize = 0;

  // Save program's name.
  getMename(&g_csMename, argv[0]);

  // Get options and dispatch errors, if any.
  getOptions(argc, argv);

  // Get all data from all files.
  for (int i = 0; i < g_tArgs.sCount; ++i) {
    hFile  = openFile(g_tArgs.pVal[i].cStr, "rb");
    liSize = getFileSize(hFile);
//-- file ----------------------------------------------------------------------
    decryptKeyAndFile(hFile, liSize);
//-- file ----------------------------------------------------------------------
    fclose(hFile);
  }

  // Free all used memory, prior end of program.
  daFreeEx(g_tArgs, cStr);

  return ERR_NOERR;
}
