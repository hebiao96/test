#ifndef IAP_MD5_H
#define IAP_MD5_H

#include <stdint.h>

/* MD5上下文结构体 */
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
} MD5_CTX;

/* MD5函数声明 */
void MD5_Init(MD5_CTX *context);
void MD5_Update(MD5_CTX *context, const uint8_t *input, uint32_t inputLen);
void MD5_Final(uint8_t digest[16], MD5_CTX *context);
void MD5_Calculate(const uint8_t *data, uint32_t length, uint8_t *md5_result);

#endif /* IAP_MD5_H */
