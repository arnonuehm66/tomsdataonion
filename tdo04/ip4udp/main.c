#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>


typedef unsigned char uchar;

typedef struct s_ip4head {
  uchar    ver : 4;
  uchar    ihl : 4;
  uchar    dscp : 6;
  uchar    ecn  : 2;
  uint16_t totalLength;
  uint16_t identification;
  uint16_t flags       : 3;
  uint16_t fragmentOff : 13;
  uchar    timetolive;
  uchar    protocol;
  uint16_t checksum;
  uchar    ipSrc[4];
  uchar    ipDst[4];
} t_ip4head;

// For checksum purposes.
typedef struct s_ip4headUdp {
  uchar    ipSrc[4];
  uchar    ipDst[4];
  uchar    zeros;
  uchar    protocol;
  uint16_t udpLen;
} t_ip4headUdp;

typedef struct s_udpHead {
  uint16_t portSrc;
  uint16_t portDst;
  uint16_t length;
  uint16_t checksum;
} t_udpHead;


//******************************************************************************
//* 1 element =  OK, 0 elements = EOF
int readBytes(void* pvBytes, int iLength, FILE* hFile) {
  size_t sBytesRead = 0;

  sBytesRead = fread(pvBytes, iLength, 1, hFile);

  return sBytesRead;
}

//******************************************************************************
int checkIp4Integrity(t_ip4head* pIp4Head, int iSize) {
  // IP-Adr: 10.1.1.10
  if (pIp4Head->ipSrc[0] !=  10) return 0;
  if (pIp4Head->ipSrc[1] !=   1) return 0;
  if (pIp4Head->ipSrc[2] !=   1) return 0;
  if (pIp4Head->ipSrc[3] !=  10) return 0;

  // IP-Adr: 10.1.1.200
  if (pIp4Head->ipDst[0] !=  10) return 0;
  if (pIp4Head->ipDst[1] !=   1) return 0;
  if (pIp4Head->ipDst[2] !=   1) return 0;
  if (pIp4Head->ipDst[3] != 200) return 0;

  return 1;
}

//******************************************************************************
int checkUdpIntegrity(t_udpHead* pUdpHead, int iSize) {
  uint16_t ui16HostDst = ntohs(pUdpHead->portDst);

  // Watch the little endianess.
  // 0x55a4 = 21924 => 42069 = 0xa455
  if (ui16HostDst != 42069) return 0;

  return 1;
}

//******************************************************************************
void createPseudoHeader(t_ip4headUdp* pip4u, t_ip4head* pip4, t_udpHead* pudp) {
  for (int i = 0; i < 4; ++i) {
    pip4u->ipSrc[i] = pip4->ipSrc[i];
    pip4u->ipDst[i] = pip4->ipDst[i];
  }
  pip4u->zeros    = 0;
  pip4u->protocol = pip4->protocol;
  pip4u->udpLen   = pudp->length;
}

//******************************************************************************
int checkSum(int iSum) {
  int iCarry = iSum & 0x000f0000;       // Get carry nibble.
      iSum   = iSum & 0x0000ffff;       // Delete carry from sum.
      iSum   = iSum + (iCarry >> 16);   // Add carry.
      iSum   = iSum ^ 0x0000ffff;       // Flip 16 bits.
  return (iSum == 0) ? 1 : 0;
}

//******************************************************************************
int checkSumIp4(uint16_t* pui16Byte, int iSize) {
  int iSum   = 0;
  int iCount = iSize / 2;

  for (int i = 0; i < iCount; ++i) iSum += pui16Byte[i];

  return checkSum(iSum);
}

//******************************************************************************
int checkSumUdp(uint16_t* pui16ip4, int iSIp4,
                uint16_t* pui16udp, int iSUdp,
                uint16_t* pui16pld, int iSPld) {
  int iSum   = 0;
  int iCIp4  = iSIp4 / 2;
  int iCUdp  = iSUdp / 2;
  int isOdd  = iSPld % 2;
  int iCPld  = iSPld / 2;   // Integer division get rid of not needed 0.5.

  // Pad last byte into an 16 bit integer with trailing zero.
  uint16_t iLastPl = 0;
  if (isOdd) iLastPl = ((uchar*) pui16pld)[iSPld - 1];

  for (int i = 0; i < iCIp4; ++i) iSum += pui16ip4[i];
  for (int i = 0; i < iCUdp; ++i) iSum += pui16udp[i];
  for (int i = 0; i < iCPld; ++i) iSum += pui16pld[i];
  if (isOdd)                      iSum += iLastPl;

  return checkSum(iSum);
}

//******************************************************************************
void printPayload(uchar* pucBytes, int iLength) {
  for (int i = 0; i < iLength; ++i)
    printf("%c", pucBytes[i]);
}


//******************************************************************************
int main(int argc, char* argv[]) {
  FILE*        hFile    = NULL;
  t_ip4head    ip4Head  = {0};
  t_ip4headUdp ip4HeadU = {0};
  t_udpHead    udpHead  = {0};
  uchar*       pucData  = NULL;
  uint16_t     ui16Len  = 0;

  if (! (hFile = fopen(argv[1], "rb"))) {
    perror("couldn't open file");
    return 1;
  }

  while (1) {
    // Get the headers and check if the file is eof.
    if (! readBytes(&ip4Head, sizeof(ip4Head), hFile)) break;
    if (! readBytes(&udpHead, sizeof(udpHead), hFile)) break;

    // Get the payload.
    ui16Len = ntohs(udpHead.length) - sizeof(udpHead);
    pucData = (uchar*) realloc(pucData, ui16Len);
    readBytes(pucData, ui16Len, hFile);

    // Check inegrity of headers.
    createPseudoHeader(&ip4HeadU, &ip4Head, &udpHead);
    if (! checkIp4Integrity(&ip4Head, sizeof(ip4Head))) continue;
    if (! checkUdpIntegrity(&udpHead, sizeof(udpHead))) continue;

    // Check checksums.
    if (! checkSumIp4((uint16_t*) &ip4Head,  sizeof(ip4Head))) continue;
    if (! checkSumUdp((uint16_t*) &ip4HeadU, sizeof(ip4HeadU),
                      (uint16_t*) &udpHead,  sizeof(udpHead),
                      (uint16_t*) pucData,   ui16Len))         continue;

    printPayload(pucData, ui16Len);
  }

  fclose(hFile);
}
