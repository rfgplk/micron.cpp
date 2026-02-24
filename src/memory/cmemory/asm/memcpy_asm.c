extern "C" {

void small_memcpy_sse_16(void *, const void *, unsigned long int);
void small_memcpy_sse_16_1(void *, const void *, unsigned long int);

void small_memcpy_sse_64(void *, const void *, unsigned long int);
void small_memcpy_sse_64_1(void *, const void *, unsigned long int);

void small_memcpy_sse_128(void *, const void *, unsigned long int);
void small_memcpy_sse_128_1(void *, const void *, unsigned long int);

void small_memcpy_avx_16(void *, const void *, unsigned long int);
void small_memcpy_avx_16_1(void *, const void *, unsigned long int);

void small_memcpy_avx_64(void *, const void *, unsigned long int);
void small_memcpy_avx_64_1(void *, const void *, unsigned long int);

void small_memcpy_avx_128(void *, const void *, unsigned long int);
void small_memcpy_avx_128_1(void *, const void *, unsigned long int);

void small_memcpy_avx2_64(void *, const void *, unsigned long int);
void small_memcpy_avx2_64_1(void *, const void *, unsigned long int);

void small_memcpy_avx2_128(void *, const void *, unsigned long int);
void small_memcpy_avx2_128_1(void *, const void *, unsigned long int);
};
