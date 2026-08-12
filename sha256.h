#ifndef SHA256_H
#define SHA256_H

/* =====================================================
   Self-contained SHA-256 implementation.

   Rationale for writing our own instead of linking
   OpenSSL/libcrypto: this is a MISRA-C:2012 target and
   the project intentionally has zero third-party
   dependencies beyond the C standard library and
   pthreads (see common.h). Pulling in libcrypto would
   add an unaudited external dependency and an ABI/version
   coupling that is hard to defend in a MISRA deviation
   record. This implementation is small enough to review
   in full and contains no dynamic memory allocation.
   ===================================================== */

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_BYTES   32U
#define SHA256_HEX_CHARS      65U   /* 64 hex chars + NUL   */
#define SHA256_SALT_BYTES     16U
#define SHA256_SALT_HEX_CHARS 33U   /* 32 hex chars + NUL   */

typedef struct
{
    uint32_t state[8];
    uint64_t bitLength;
    uint8_t  buffer[64];
    uint32_t bufferLength;
} Sha256Context;

void sha256Init(Sha256Context *ctx);
void sha256Update(Sha256Context *ctx, const uint8_t *data, size_t length);
void sha256Final(Sha256Context *ctx, uint8_t digest[SHA256_DIGEST_BYTES]);

/* Convenience one-shot: hashes (salt || password) and writes the
   result as a lowercase hex string (SHA256_HEX_CHARS bytes incl NUL)
   into outHex. */
void sha256HashSalted(const char *saltHex,
                       const char *password,
                       char outHex[SHA256_HEX_CHARS]);

/* Fills outSaltHex with SHA256_SALT_BYTES worth of random bytes,
   encoded as lowercase hex, NUL-terminated. Returns 1 on success,
   0 on failure (e.g. the platform RNG source could not be read). */
int sha256GenerateSaltHex(char outSaltHex[SHA256_SALT_HEX_CHARS]);

/* Constant-time comparison of two NUL-terminated hex digest strings
   of length SHA256_HEX_CHARS - 1. Returns 1 if equal, 0 otherwise.
   Used instead of strcmp() when comparing password hashes, so that
   the time taken to compare does not leak how many leading hex
   characters matched (a standard defence against timing attacks on
   authentication checks). */
int sha256ConstantTimeEqual(const char *hexA, const char *hexB);

#endif
