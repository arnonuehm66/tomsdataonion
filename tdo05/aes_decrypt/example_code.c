#include <inttypes.h>
//#include <algorithm>
#include <openssl/aes.h>
#include <openssl/rand.h>

void test_aes(void) {
  uint8_t Key[32];
  uint8_t IV[AES_BLOCK_SIZE];
  uint8_t IVd[AES_BLOCK_SIZE];

  RAND_bytes(Key, sizeof(Key));   // Generate an AES Key
  RAND_bytes(IV , sizeof(IV) );   // and Initialization Vector

  // Make a copy of the IV to IVd as it seems to get destroyed when used
  for(int i=0; i < AES_BLOCK_SIZE; i++) IVd[i] = IV[i];

  /** Setup the AES Key structure required for use in the OpenSSL APIs **/
  AES_KEY* AesKey = {0};
  AES_set_encrypt_key(Key, 256, AesKey);

  /** take an input string and pad it so it fits into 16 bytes (AES Block Size) **/
  char* txt ="this is a test";

  const int UserDataSize = (const int)txt.length();                           // Get the length pre-padding
  int RequiredPadding = (AES_BLOCK_SIZE - (txt.length() % AES_BLOCK_SIZE));   // Calculate required padding
  std::vector<unsigned char> PaddedTxt(txt.begin(), txt.end());               // Easier to Pad as a vector

  for(int i=0; i < RequiredPadding; i++)
      PaddedTxt.push_back(0); //  Increase the size of the string by how much padding is necessary.

  unsigned char * UserData = &PaddedTxt[0];// Get the padded text as an unsigned char array
  const int UserDataSizePadded = (const int)PaddedTxt.size();// and the length (OpenSSl is a C-API)

  /** Peform the encryption **/
  unsigned char EncryptedData[512] = {0}; // Hard-coded Array for OpenSSL (C++ can't dynamic arrays)
  AES_cbc_encrypt(UserData, EncryptedData, UserDataSizePadded, (const AES_KEY*)AesKey, IV, AES_ENCRYPT);

  /** Setup an AES Key structure for the decrypt operation **/
  AES_KEY* AesDecryptKey = new AES_KEY(); // AES Key to be used for Decryption
  AES_set_decrypt_key(Key, 256, AesDecryptKey);   // We Initialize this so we can use the OpenSSL Encryption API

  /** Decrypt the data. Note that we use the same function call. Only change is the last parameter **/
  unsigned char DecryptedData[512] = {0}; // Hard-coded as C++ doesn't allow for dynamic arrays and OpenSSL requires an array
  AES_cbc_encrypt(EncryptedData, DecryptedData, UserDataSizePadded, (const AES_KEY*)AesDecryptKey, IVd, AES_DECRYPT);
}
