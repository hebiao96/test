#include "iap_main.h"
#include <string.h>

/* MD5算法常量 */
#define MD5_S11 7
#define MD5_S12 12
#define MD5_S13 17
#define MD5_S14 22
#define MD5_S21 5
#define MD5_S22 9
#define MD5_S23 14
#define MD5_S24 20
#define MD5_S31 4
#define MD5_S32 11
#define MD5_S33 16
#define MD5_S34 23
#define MD5_S41 6
#define MD5_S42 10
#define MD5_S43 15
#define MD5_S44 21

/* MD5算法辅助函数 */
#define MD5_F(x,y,z) (((x) & (y)) | ((~x) & (z)))
#define MD5_G(x,y,z) (((x) & (z)) | ((y) & (~z)))
#define MD5_H(x,y,z) ((x) ^ (y) ^ (z))
#define MD5_I(x,y,z) ((y) ^ ((x) | (~z)))

#define MD5_ROTATE_LEFT(x,n) (((x) << (n)) | ((x) >> (32-(n))))

#define MD5_FF(a,b,c,d,x,s,ac) { \
    (a) += MD5_F ((b),(c),(d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

#define MD5_GG(a,b,c,d,x,s,ac) { \
    (a) += MD5_G ((b),(c),(d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

#define MD5_HH(a,b,c,d,x,s,ac) { \
    (a) += MD5_H ((b),(c),(d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

#define MD5_II(a,b,c,d,x,s,ac) { \
    (a) += MD5_I ((b),(c),(d)) + (x) + (uint32_t)(ac); \
    (a) = MD5_ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
}

/**
 * @brief MD5初始化
 */
void MD5_Init(MD5_CTX *context)
{
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

/**
 * @brief MD5数据块处理
 */
static void MD5_Transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    uint32_t i;
    
    /* 转换字节序 */
    for (i = 0; i < 16; i++) {
        x[i] = ((uint32_t)block[i*4]) | (((uint32_t)block[i*4+1]) << 8) |
               (((uint32_t)block[i*4+2]) << 16) | (((uint32_t)block[i*4+3]) << 24);
    }
    
    /* Round 1 */
    MD5_FF (a, b, c, d, x[ 0], MD5_S11, 0xd76aa478);
    MD5_FF (d, a, b, c, x[ 1], MD5_S12, 0xe8c7b756);
    MD5_FF (c, d, a, b, x[ 2], MD5_S13, 0x242070db);
    MD5_FF (b, c, d, a, x[ 3], MD5_S14, 0xc1bdceee);
    MD5_FF (a, b, c, d, x[ 4], MD5_S11, 0xf57c0faf);
    MD5_FF (d, a, b, c, x[ 5], MD5_S12, 0x4787c62a);
    MD5_FF (c, d, a, b, x[ 6], MD5_S13, 0xa8304613);
    MD5_FF (b, c, d, a, x[ 7], MD5_S14, 0xfd469501);
    MD5_FF (a, b, c, d, x[ 8], MD5_S11, 0x698098d8);
    MD5_FF (d, a, b, c, x[ 9], MD5_S12, 0x8b44f7af);
    MD5_FF (c, d, a, b, x[10], MD5_S13, 0xffff5bb1);
    MD5_FF (b, c, d, a, x[11], MD5_S14, 0x895cd7be);
    MD5_FF (a, b, c, d, x[12], MD5_S11, 0x6b901122);
    MD5_FF (d, a, b, c, x[13], MD5_S12, 0xfd987193);
    MD5_FF (c, d, a, b, x[14], MD5_S13, 0xa679438e);
    MD5_FF (b, c, d, a, x[15], MD5_S14, 0x49b40821);
    
    /* Round 2 */
    MD5_GG (a, b, c, d, x[ 1], MD5_S21, 0xf61e2562);
    MD5_GG (d, a, b, c, x[ 6], MD5_S22, 0xc040b340);
    MD5_GG (c, d, a, b, x[11], MD5_S23, 0x265e5a51);
    MD5_GG (b, c, d, a, x[ 0], MD5_S24, 0xe9b6c7aa);
    MD5_GG (a, b, c, d, x[ 5], MD5_S21, 0xd62f105d);
    MD5_GG (d, a, b, c, x[10], MD5_S22,  0x2441453);
    MD5_GG (c, d, a, b, x[15], MD5_S23, 0xd8a1e681);
    MD5_GG (b, c, d, a, x[ 4], MD5_S24, 0xe7d3fbc8);
    MD5_GG (a, b, c, d, x[ 9], MD5_S21, 0x21e1cde6);
    MD5_GG (d, a, b, c, x[14], MD5_S22, 0xc33707d6);
    MD5_GG (c, d, a, b, x[ 3], MD5_S23, 0xf4d50d87);
    MD5_GG (b, c, d, a, x[ 8], MD5_S24, 0x455a14ed);
    MD5_GG (a, b, c, d, x[13], MD5_S21, 0xa9e3e905);
    MD5_GG (d, a, b, c, x[ 2], MD5_S22, 0xfcefa3f8);
    MD5_GG (c, d, a, b, x[ 7], MD5_S23, 0x676f02d9);
    MD5_GG (b, c, d, a, x[12], MD5_S24, 0x8d2a4c8a);
    
    /* Round 3 */
    MD5_HH (a, b, c, d, x[ 5], MD5_S31, 0xfffa3942);
    MD5_HH (d, a, b, c, x[ 8], MD5_S32, 0x8771f681);
    MD5_HH (c, d, a, b, x[11], MD5_S33, 0x6d9d6122);
    MD5_HH (b, c, d, a, x[14], MD5_S34, 0xfde5380c);
    MD5_HH (a, b, c, d, x[ 1], MD5_S31, 0xa4beea44);
    MD5_HH (d, a, b, c, x[ 4], MD5_S32, 0x4bdecfa9);
    MD5_HH (c, d, a, b, x[ 7], MD5_S33, 0xf6bb4b60);
    MD5_HH (b, c, d, a, x[10], MD5_S34, 0xbebfbc70);
    MD5_HH (a, b, c, d, x[13], MD5_S31, 0x289b7ec6);
    MD5_HH (d, a, b, c, x[ 0], MD5_S32, 0xeaa127fa);
    MD5_HH (c, d, a, b, x[ 3], MD5_S33, 0xd4ef3085);
    MD5_HH (b, c, d, a, x[ 6], MD5_S34,  0x4881d05);
    MD5_HH (a, b, c, d, x[ 9], MD5_S31, 0xd9d4d039);
    MD5_HH (d, a, b, c, x[12], MD5_S32, 0xe6db99e5);
    MD5_HH (c, d, a, b, x[15], MD5_S33, 0x1fa27cf8);
    MD5_HH (b, c, d, a, x[ 2], MD5_S34, 0xc4ac5665);
    
    /* Round 4 */
    MD5_II (a, b, c, d, x[ 0], MD5_S41, 0xf4292244);
    MD5_II (d, a, b, c, x[ 7], MD5_S42, 0x432aff97);
    MD5_II (c, d, a, b, x[14], MD5_S43, 0xab9423a7);
    MD5_II (b, c, d, a, x[ 5], MD5_S44, 0xfc93a039);
    MD5_II (a, b, c, d, x[12], MD5_S41, 0x655b59c3);
    MD5_II (d, a, b, c, x[ 3], MD5_S42, 0x8f0ccc92);
    MD5_II (c, d, a, b, x[10], MD5_S43, 0xffeff47d);
    MD5_II (b, c, d, a, x[ 1], MD5_S44, 0x85845dd1);
    MD5_II (a, b, c, d, x[ 8], MD5_S41, 0x6fa87e4f);
    MD5_II (d, a, b, c, x[15], MD5_S42, 0xfe2ce6e0);
    MD5_II (c, d, a, b, x[ 6], MD5_S43, 0xa3014314);
    MD5_II (b, c, d, a, x[13], MD5_S44, 0x4e0811a1);
    MD5_II (a, b, c, d, x[ 4], MD5_S41, 0xf7537e82);
    MD5_II (d, a, b, c, x[11], MD5_S42, 0xbd3af235);
    MD5_II (c, d, a, b, x[ 2], MD5_S43, 0x2ad7d2bb);
    MD5_II (b, c, d, a, x[ 9], MD5_S44, 0xeb86d391);
    
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

/**
 * @brief MD5更新数据
 */
void MD5_Update(MD5_CTX *context, const uint8_t *input, uint32_t inputLen)
{
    uint32_t i, index, partLen;
    
    /* 计算已处理的字节数 */
    index = (uint32_t)((context->count[0] >> 3) & 0x3F);
    
    /* 更新比特计数 */
    if ((context->count[0] += ((uint32_t)inputLen << 3)) < ((uint32_t)inputLen << 3))
        context->count[1]++;
    context->count[1] += ((uint32_t)inputLen >> 29);
    
    partLen = 64 - index;
    
    /* 处理尽可能多的64字节块 */
    if (inputLen >= partLen) {
        memcpy((uint8_t*)&context->buffer[index], input, partLen);
        MD5_Transform(context->state, context->buffer);
        
        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5_Transform(context->state, &input[i]);
        
        index = 0;
    } else {
        i = 0;
    }
    
    /* 缓存剩余数据 */
    memcpy((uint8_t*)&context->buffer[index], &input[i], inputLen - i);
}

/**
 * @brief MD5计算最终结果
 */
void MD5_Final(uint8_t digest[16], MD5_CTX *context)
{
    uint8_t bits[8];
    uint32_t index, padLen;
    uint8_t padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t i;
    
    /* 保存长度信息 */
    for (i = 0; i < 8; i++) {
        bits[i] = (uint8_t)((context->count[i>>2] >> ((i & 0x3) << 3)) & 0xFF);
    }
    
    /* 填充数据 */
    index = (uint32_t)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5_Update(context, padding, padLen);
    
    /* 添加长度信息 */
    MD5_Update(context, bits, 8);
    
    /* 存储最终结果 */
    for (i = 0; i < 16; i++) {
        digest[i] = (uint8_t)((context->state[i>>2] >> ((i & 0x3) << 3)) & 0xFF);
    }
}

/**
 * @brief 计算数据的MD5值（一次性完成）
 * @param data 输入数据指针
 * @param length 数据长度
 * @param md5_result 输出MD5结果（16字节）
 */
void MD5_Calculate(const uint8_t *data, uint32_t length, uint8_t *md5_result)
{
    MD5_CTX context;
    
    MD5_Init(&context);
    MD5_Update(&context, data, length);
    MD5_Final(md5_result, &context);
}
