#include <string.h>
#include <stdio.h>

#include "sha256.h"

/* Public-domain-style SHA-256 (FIPS 180-4). Written from the
   specification, self-contained, no dynamic allocation. Every
   function has a single exit point and every control-statement
   body is braced, per the project's MISRA-C:2012 conventions
   (see MISRA_DEVIATIONS.md). */

static const uint32_t K[64] =
{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static uint32_t rotr32(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32U - n));
}

static void sha256ProcessBlock(Sha256Context *ctx, const uint8_t block[64])
{
    uint32_t w[64];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    uint32_t g;
    uint32_t h;
    uint32_t i;

    for(i = 0U; i < 16U; i++)
    {
        w[i] = ((uint32_t)block[(i * 4U)] << 24) |
               ((uint32_t)block[(i * 4U) + 1U] << 16) |
               ((uint32_t)block[(i * 4U) + 2U] << 8) |
               (uint32_t)block[(i * 4U) + 3U];
    }

    for(i = 16U; i < 64U; i++)
    {
        uint32_t s0 = rotr32(w[i - 15U], 7U) ^ rotr32(w[i - 15U], 18U) ^ (w[i - 15U] >> 3);
        uint32_t s1 = rotr32(w[i - 2U], 17U) ^ rotr32(w[i - 2U], 19U) ^ (w[i - 2U] >> 10);

        w[i] = w[i - 16U] + s0 + w[i - 7U] + s1;
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for(i = 0U; i < 64U; i++)
    {
        uint32_t s1 = rotr32(e, 6U) ^ rotr32(e, 11U) ^ rotr32(e, 25U);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + s1 + ch + K[i] + w[i];
        uint32_t s0 = rotr32(a, 2U) ^ rotr32(a, 13U) ^ rotr32(a, 22U);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256Init(Sha256Context *ctx)
{
    if(ctx != NULL)
    {
        ctx->state[0] = 0x6a09e667U;
        ctx->state[1] = 0xbb67ae85U;
        ctx->state[2] = 0x3c6ef372U;
        ctx->state[3] = 0xa54ff53aU;
        ctx->state[4] = 0x510e527fU;
        ctx->state[5] = 0x9b05688cU;
        ctx->state[6] = 0x1f83d9abU;
        ctx->state[7] = 0x5be0cd19U;
        ctx->bitLength = 0U;
        ctx->bufferLength = 0U;
        (void)memset(ctx->buffer, 0, sizeof(ctx->buffer));
    }
}

void sha256Update(Sha256Context *ctx, const uint8_t *data, size_t length)
{
    if((ctx != NULL) && (data != NULL))
    {
        size_t i;

        for(i = 0U; i < length; i++)
        {
            ctx->buffer[ctx->bufferLength] = data[i];
            ctx->bufferLength++;

            if(ctx->bufferLength == 64U)
            {
                sha256ProcessBlock(ctx, ctx->buffer);
                ctx->bitLength += 512U;
                ctx->bufferLength = 0U;
            }
        }
    }
}

void sha256Final(Sha256Context *ctx, uint8_t digest[SHA256_DIGEST_BYTES])
{
    if((ctx != NULL) && (digest != NULL))
    {
        uint32_t i;
        uint64_t totalBits;

        totalBits = ctx->bitLength + ((uint64_t)ctx->bufferLength * 8U);

        ctx->buffer[ctx->bufferLength] = 0x80U;
        ctx->bufferLength++;

        if(ctx->bufferLength > 56U)
        {
            while(ctx->bufferLength < 64U)
            {
                ctx->buffer[ctx->bufferLength] = 0x00U;
                ctx->bufferLength++;
            }

            sha256ProcessBlock(ctx, ctx->buffer);
            ctx->bufferLength = 0U;
        }

        while(ctx->bufferLength < 56U)
        {
            ctx->buffer[ctx->bufferLength] = 0x00U;
            ctx->bufferLength++;
        }

        for(i = 0U; i < 8U; i++)
        {
            ctx->buffer[56U + i] = (uint8_t)(totalBits >> (56U - (8U * i)));
        }

        sha256ProcessBlock(ctx, ctx->buffer);

        for(i = 0U; i < 8U; i++)
        {
            digest[(i * 4U)] = (uint8_t)(ctx->state[i] >> 24);
            digest[(i * 4U) + 1U] = (uint8_t)(ctx->state[i] >> 16);
            digest[(i * 4U) + 2U] = (uint8_t)(ctx->state[i] >> 8);
            digest[(i * 4U) + 3U] = (uint8_t)(ctx->state[i]);
        }
    }
}

static void bytesToHex(const uint8_t *bytes, size_t byteCount, char *outHex)
{
    static const char hexDigits[16] = "0123456789abcdef";
    size_t i;

    for(i = 0U; i < byteCount; i++)
    {
        outHex[(i * 2U)] = hexDigits[(bytes[i] >> 4) & 0x0FU];
        outHex[(i * 2U) + 1U] = hexDigits[bytes[i] & 0x0FU];
    }

    outHex[(byteCount * 2U)] = '\0';
}

void sha256HashSalted(const char *saltHex, const char *password, char outHex[SHA256_HEX_CHARS])
{
    Sha256Context ctx;

    if((saltHex != NULL) && (password != NULL) && (outHex != NULL))
    {
        uint8_t digest[SHA256_DIGEST_BYTES];

        sha256Init(&ctx);
        sha256Update(&ctx, (const uint8_t *)saltHex, strlen(saltHex));
        sha256Update(&ctx, (const uint8_t *)password, strlen(password));
        sha256Final(&ctx, digest);
        bytesToHex(digest, SHA256_DIGEST_BYTES, outHex);
    }
}

int sha256GenerateSaltHex(char outSaltHex[SHA256_SALT_HEX_CHARS])
{
    int result = 0;

    if(outSaltHex != NULL)
    {
        uint8_t raw[SHA256_SALT_BYTES];
        FILE *randSource;

        randSource = fopen("/dev/urandom", "rb");

        if(randSource != NULL)
        {
            size_t itemsRead;

            itemsRead = fread(raw, 1U, SHA256_SALT_BYTES, randSource);
            (void)fclose(randSource);

            if(itemsRead == SHA256_SALT_BYTES)
            {
                bytesToHex(raw, SHA256_SALT_BYTES, outSaltHex);
                result = 1;
            }
        }
    }

    return result;
}

int sha256ConstantTimeEqual(const char *hexA, const char *hexB)
{
    int result;

    if((hexA == NULL) || (hexB == NULL))
    {
        result = 0;
    }
    else
    {
        size_t lenA = strlen(hexA);
        size_t lenB = strlen(hexB);

        if(lenA != lenB)
        {
            result = 0;
        }
        else
        {
            size_t i;
            unsigned char diff = 0U;

            for(i = 0U; i < lenA; i++)
            {
                diff |= (unsigned char)((unsigned char)hexA[i] ^ (unsigned char)hexB[i]);
            }

            result = (diff == 0U) ? 1 : 0;
        }
    }

    return result;
}
