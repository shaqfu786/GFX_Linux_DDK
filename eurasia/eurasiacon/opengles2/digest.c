/*************************************************************************/ /*!
@Title          Crypto functions for blob cache key generation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Strictly Confidential.
*/ /**************************************************************************/

#include <string.h> // for strlen
#include <stdio.h> // for sprintf

#include <openssl/sha.h>

#include "img_types.h"
#include "digest.h"

static void SHA256HashString(IMG_BYTE aucHash[SHA256_DIGEST_LENGTH],
							 IMG_CHAR szHashStr[DIGEST_STRING_LENGTH])
{
    IMG_INT i = 0;

    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(szHashStr + (i * 2), "%02x", aucHash[i]);
    }
    szHashStr[64] = 0;
}

IMG_INTERNAL
void DigestTextToHashString(const IMG_CHAR *szString,
							IMG_CHAR szHashStr[DIGEST_STRING_LENGTH])
{
	IMG_BYTE aucHash[SHA256_DIGEST_LENGTH];
	SHA256_CTX pSHA256;

	SHA256_Init(&pSHA256);
	SHA256_Update(&pSHA256, szString, strlen(szString));
	SHA256_Final(aucHash, &pSHA256);

	SHA256HashString(aucHash, szHashStr);
}
