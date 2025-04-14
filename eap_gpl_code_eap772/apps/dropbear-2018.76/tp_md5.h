#ifndef __MD5_INCLUDE__

/* typedef a 32-bit type */
#ifdef _LP64
typedef unsigned int UINT4;
typedef int          INT4;
#else
typedef unsigned long UINT4;
typedef long          INT4;
#endif
#define _UINT4_T

#ifndef bcopy
#define bcopy memcpy
#endif

/* Data structure for MD5 (Message-Digest) computation */
typedef struct {
  UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
  UINT4 buf[4];                                    /* scratch buffer */
  unsigned char in[64];                              /* input buffer */
  unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;

void MD5_Init ();
void MD5_Update ();
void MD5_Final ();

#define DES_ENCRYPT			1
#define DES_DECRYPT			0
#define MD5_DIGEST_LEN		16
#define MANAGE_USER_PWD_LENGTH   16
#define MANAGE_USER_NAME_LENGTH   16


void cal_md5(unsigned char* output, unsigned char* input, int len);

void md5_make_digest(unsigned char* digest, unsigned char* input, int len);


#define __MD5_INCLUDE__
#endif /* __MD5_INCLUDE__ */

