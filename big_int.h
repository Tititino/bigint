#ifndef BIG_INT_H
#define BIG_INT_H

typedef struct bigInt* BigInt;
typedef unsigned long long Chunk;

BigInt new_big_int(char* s);
BigInt multiply_big_int_small_int(BigInt a, Chunk n);
void print_big_int(BigInt n);
BigInt multiply_big_int(BigInt a, BigInt b);
char* big_int_to_string(BigInt n);
void destroy_big_int(BigInt n);
int big_int_change_def_base(Chunk);


#endif
