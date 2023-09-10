/*******************************************************************************
 ** Name: ascii85
 ** Purpose: Prints ascii85 decoded data to stdout.
 ** Author: (JE) Jens Elstner <jens.elstner@bka.bund.de>
 *******************************************************************************
 ** Date        User  Log
 **-----------------------------------------------------------------------------
 ** 02.06.2020  JE    Created program.
 ** 05.07.2020  JE    Added '-e' for encode input data to stdout.
 ** 05.07.2020  JE    Added '-o' for setting byte offset before reading file(s).
 ** 05.07.2020  JE    Added '-n' for printing a newline after end of program.
 ** 05.07.2020  JE    Added the ability to read from pipes.
 ** 16.07.2020  JE    Adjusted usage to reflect use in pipes is posssible.
 ** 20.10.2020  JE    Changed ftell() and fseek() to 64 bit type now use off_t.
 ** 20.10.2020  JE    Changed all size_t to off_t accordingly.
 ** 12.08.2023  JE    Now uses 'c_dynamic_arrays_macros.h' and latest libs.
 *******************************************************************************/


//******************************************************************************
//* includes & namespaces

#define _FILE_OFFSET_BITS 64  // Switch ...
#include <stdio.h>            // ... now use 64 bit with ftello() and fseeko().
#include <stdlib.h>
#include <string.h>

#include "c_string.h"
#include "c_dynamic_arrays_macros.h"


//******************************************************************************
//* me and myself

#define ME_VERSION "0.3.3"
cstr g_csMename;


//******************************************************************************
//* defines & macros

#define ERR_NOERR 0x00
#define ERR_ARGS  0x01
#define ERR_FILE  0x02
#define ERR_ELSE  0xff

#define sERR_ARGS  "Argument error"
#define sERR_FILE  "File error"
#define sERR_ELSE  "Unknown error"

// ASCII85 special chars.
#define C_MIN ('!')
#define C_MAX ('u')
#define C_NUL ('z')
#define C_EOB ('~')


//******************************************************************************
//* outsourced standard functions, includes and defines

#include "stdfcns.c"


//******************************************************************************
//* typedefs

// Arguments and options.
typedef struct s_options {
  int   iPrtEol;
  int   iEncode;
  off_t oOffset;
  int   iReadStdin;
} t_options;

// For conversion of int32 into its bytes and vice versa.
typedef union s_i32c {
  uint32_t u32Int;
  uchar    aChars[4];
} t_i32c;

s_array(cstr);
s_array(int);


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
  "usage: %s [-n] [-e] [-o n] file1 [file2 ...]\n"
  "       %s [-h|--help|-v|--version]\n"
  " Reads file(s) and prints ascii85 decoded/encoded data to stdout.\n"
  " Data can also been piped into the program. Examples:\n"
  "   >$ echo '<~ARTY*$3~>' | %s\n"
  "   easy\n"
  "   >$ echo easy | %s -en\n"
  "   <~ARTY*$3~>\n"
  "  -n:            print a newline after conversion\n"
  "  -e:            encode bytes to ascii85 (default decodes to bytes)\n"
  "  -o n:          set byte offset where file(s) start to be read\n"
  "  -h|--help:     print this help\n"
  "  -v|--version:  print version of program\n"
//|************************ 80 chars width ****************************************|
         ,csMsg.cStr,
         g_csMename.cStr, g_csMename.cStr, g_csMename.cStr, g_csMename.cStr
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

  if (rv == ERR_ARGS) csSet(&csErr, sERR_ARGS);
  if (rv == ERR_FILE) csSet(&csErr, sERR_FILE);
  if (rv == ERR_ELSE) csSet(&csErr, sERR_ELSE);

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
  g_tOpts.iPrtEol    = 0;
  g_tOpts.iEncode    = 0;
  g_tOpts.oOffset    = 0;
  g_tOpts.iReadStdin = 0;

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
        if (cOpt == 'n') {
          g_tOpts.iPrtEol = 1;
          continue;
        }
        if (cOpt == 'e') {
          g_tOpts.iEncode = 1;
          continue;
        }
        if (cOpt == 'o') {
          if (! getArgHexLong((ll*) &g_tOpts.oOffset, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid offset or missing");
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
  if (g_tOpts.oOffset < 0) dispatchError(ERR_ARGS, "Offset < 0");

  // Switch to stdin if no files were given.
  if (g_tArgs.sCount  == 0) g_tOpts.iReadStdin = 1;

  // Free string memory.
  csFree(&csArgv);
  csFree(&csRv);
  csFree(&csOpt);
}

//******************************************************************************
//*** decoder

/*******************************************************************************
 * Name:  readASCII852Array
 * Purpose: Reads all chars between '<~' & '~>' into array, skips whitespaces.
 *******************************************************************************/
int readAscii852array(t_array(int)* pdaiBytes, FILE* hFile) {
  int b = 0;

  // Read bytes until '<~' appears.
  while (fread(&b, sizeof(char), 1, hFile)) {
    if (b != '<') continue;
    fread(&b, sizeof(char), 1, hFile);
    if (b == '~') break;
  }

  // Save to array until '~' appears (i.e. "~>")
  while (fread(&b, sizeof(char), 1, hFile)) {
    if (b == C_EOB)              return 1;
    if (b == C_NUL)              for (int i = 0; i < 5; ++i) daAdd(int, (*pdaiBytes), C_MIN);
    if (b <  C_MIN || b > C_MAX) continue;
    daAdd(int, (*pdaiBytes), b);
  }

  return 0;
}

/*******************************************************************************
 * Name:  padArrayTo5
 * Purpose: Pads array with 'u' so its length is a multiple of 5.
 *******************************************************************************/
int padArrayTo5(t_array(int)* pdaiBytes) {
  int iPad = 5 - (pdaiBytes->sCount) % 5;

  if (iPad == 5) return 0;

  // Padding is like follows:
  // Chars:   0 1 2 3 4 5 6 7 8 9 10
  // Padding: 0 4 3 2 1 0 4 3 2 1 0

  for (int i = 0; i < iPad; ++i)
    daAdd(int, (*pdaiBytes), C_MAX);

  return iPad;
}

/*******************************************************************************
 * Name:  get4IntFrom5Block
 * Purpose: Converts 5 bytes into a 4 byte integer.
 *******************************************************************************/
void get4IntFrom5Block(uint32_t* puInt32, t_array(int)* pdaiBytes, off_t* poOff) {
  int n[5] = {0};

  // Read 5 bytes from array.
  for (int i = 0; i < 5; ++i)
    n[i] = pdaiBytes->pVal[*poOff + i];

  // Calculation of 4 Bytes from five chars.
  *puInt32 = (n[0] - 33) * 85 * 85 * 85 * 85
           + (n[1] - 33) * 85 * 85 * 85
           + (n[2] - 33) * 85 * 85
           + (n[3] - 33) * 85
           + (n[4] - 33);

  *poOff += 5;
}

/*******************************************************************************
 * Name:  print4int
 * Purpose: Prints the int as 4 bytes little-endian.
 *******************************************************************************/
void print4int(uint32_t u32Int, int iMax) {
  t_i32c ui32c = {0};

  ui32c.u32Int = u32Int;
  for (int i = 0; i < iMax; ++i) {
    printf("%c", ui32c.aChars[3 - i]);
  }
}

/*******************************************************************************
 * Name:  ascii852bin
 * Purpose: Converts an ascii84 data stream to a byte stream.
 *******************************************************************************/
void ascii852bin(FILE* hFile) {
  uint32_t     uInt32   = {0};
  t_array(int) daiBytes = {0};
  off_t        oOff     = g_tOpts.oOffset;
  int          iPad     = 0;
  int          iN       = 4;

  daInit(int, daiBytes);

  if (! readAscii852array(&daiBytes, hFile))
    dispatchError(ERR_FILE, "Error reading file");

  iPad = padArrayTo5(&daiBytes);

  while(1) {
    get4IntFrom5Block(&uInt32, &daiBytes, &oOff);
    if (oOff == daiBytes.sCount) iN = 4 - iPad;
    print4int(uInt32, iN);
    if (oOff >= daiBytes.sCount) break;
  }

  // Print an end of line, if wanted.
  if(g_tOpts.iPrtEol) printf("\n");

  daFree(daiBytes);
}

//*** decoder
//******************************************************************************


//******************************************************************************
//*** encoder

/*******************************************************************************
 * Name:  readASCII852Array
 * Purpose: Reads a file into a dynamic array.
 *******************************************************************************/
int readFile2array(t_array(int)* pdaiBytes, FILE* hFile) {
  int b = 0;

  // Read bytes until eof.
  while (fread(&b, sizeof(char), 1, hFile))
    daAdd(int, (*pdaiBytes), b);

  return 1;
}

/*******************************************************************************
 * Name:  padArrayTo4
 * Purpose: Pads array with 0 so its length is a multiple of 4.
 *******************************************************************************/
int padArrayTo4(t_array(int)* pdaiBytes) {
  int iPad = 4 - (pdaiBytes->sCount) % 4;

  if (iPad == 4) return 0;

  // Padding is like follows:
  // Chars:   0 1 2 3 4 5 6 7 8
  // Padding: 0 3 2 1 0 3 2 1 0

  for (int i = 0; i < iPad; ++i)
    daAdd(int, (*pdaiBytes), 0);

  return iPad;
}

/*******************************************************************************
 * Name:  get5BlockFrom4Int
 * Purpose: Converts a 4 byte integer into 5 bytes.
 *******************************************************************************/
void get5BlockFrom4Int(int* n, t_array(int)* pdaiBytes, off_t* poOff) {
  t_i32c ui32c = {0};

  // Read 4 bytes from array.
  for (int i = 0; i < 4; ++i)
    ui32c.aChars[3 - i] = pdaiBytes->pVal[*poOff + i];

  // Get the 5 chars out of the integer.
  n[0] = ((ui32c.u32Int / (85 * 85 * 85 * 85)) % 85) + 33;
  n[1] = ((ui32c.u32Int / (85 * 85 * 85     )) % 85) + 33;
  n[2] = ((ui32c.u32Int / (85 * 85          )) % 85) + 33;
  n[3] = ((ui32c.u32Int / (85               )) % 85) + 33;
  n[4] = ((ui32c.u32Int                      ) % 85) + 33;

  *poOff += 4;
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void print5chars(int* b, int iMax) {
  for (int i = 0; i < iMax; ++i)
    printf("%c", b[i]);
}

/*******************************************************************************
 * Name:  .
 * Purpose: .
 *******************************************************************************/
void bin2ascii85(FILE* hFile) {
  int          n[5]     = {0};
  t_array(int) daiBytes = {0};
  off_t        oOff     = g_tOpts.oOffset;
  int          iPad     = 0;
  int          iN       = 5;

  daInit(int, daiBytes);

  // Read the file into a byte array
  if (! readFile2array(&daiBytes, hFile))
    dispatchError(ERR_FILE, "Error reading file");

  // Pad to mod 4 and save pad count
  iPad = padArrayTo4(&daiBytes);

  printf("<~");
  while(1) {
    get5BlockFrom4Int(n, &daiBytes, &oOff);
    if (oOff == daiBytes.sCount) iN = 5 - iPad;
    print5chars(n, iN);
    if (oOff >= daiBytes.sCount) break;
  }
  printf("~>");

  // Print an end of line, if wanted.
  if(g_tOpts.iPrtEol) printf("\n");

  daFree(daiBytes);
}

//*** encoder
//******************************************************************************


//******************************************************************************
//* main

int main(int argc, char *argv[]) {
  FILE* hFile  = NULL;
  off_t oSize  = 0;
  int   iStdin = 0;

  // Save program's name.
  g_csMename = csNew("");
  getMename(&g_csMename, argv[0]);

  // Get options and dispatch errors, if any.
  getOptions(argc, argv);

  // If to use stdin instead of files, say so.
  iStdin = g_tOpts.iReadStdin;

  // Get all data from all files, or stdin.
  for (int i = 0; i < g_tArgs.sCount || iStdin; ++i) {
    if (iStdin) {
      hFile  = stdin;
      iStdin = 0;
    }
    else {
      hFile = openFile(g_tArgs.pVal[i].cStr, "rb");
      oSize = getFileSize(hFile);
      if (g_tOpts.oOffset > oSize)
        dispatchError(ERR_FILE, "Offset greater than file size");
      fseeko(hFile, g_tOpts.oOffset, SEEK_SET);
    }
//-- file ----------------------------------------------------------------------
    if (! g_tOpts.iEncode) ascii852bin(hFile);
    if (  g_tOpts.iEncode) bin2ascii85(hFile);
//-- file ----------------------------------------------------------------------
    fclose(hFile);
  }

  // Free all used memory, prior end of program.
  daFreeEx(g_tArgs, cStr);

  return ERR_NOERR;
}
