#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

// #define BASE 	(unsigned long long) 1000000000000000000
#ifndef BASE
#define BASE  	(unsigned long long) 10000000
#endif


#define ALLOCATION_ERR	fprintf(stderr, "allocation error in `%s` at %s:%d\n", __func__, __FILE__, __LINE__)


// two digits
typedef unsigned long long Chunk;

// len is the total number of chunks
// there is no sign for now
// the smallest digit is on the right
// using unsigned long long ints the base is 10^19
// so each number represented by a BigInt is
// (nums[0]) * 10^19^0 + (nums[1]) * 10^19^1 + ... + (nums[i]) * 10^19^i + ... + (nums[n]) * 10^19^n
// where n is len - 1.

typedef struct bigInt {
	Chunk* nums;
	size_t len;
}* BigInt;

int has_overflown(Chunk mult);
BigInt new_big_int(char* s);
void print_big_int(BigInt n);
BigInt multiply_big_int_small_int(BigInt a, Chunk n);
BigInt multiply_big_int(BigInt a, BigInt b);
void print_big_int2(BigInt n);
int is_zero_big_int(BigInt n);
char* big_int_to_string(BigInt n);
void destroy_big_int(BigInt n);

// better check for overflows

int has_overflown(Chunk mult) {
	int res = 0;

	while (mult && (res < 2)) {
		res += (mult % 10) != 0;
		mult /= 10;
	}

	return res == 2;
}

BigInt new_big_int(char* s) {
	int string_len;
	Chunk acc, mult10, add;
	// length of the string
	for (string_len = 0; s[string_len]; string_len++);
	
	Chunk* new_nums = NULL, *temp;
	
	BigInt new = malloc(sizeof(struct bigInt));
	if (!new) {
		errno = EFAULT;
		ALLOCATION_ERR;
		exit(1);
	}
	
	new -> len = 0;

	acc = 0;
	mult10 = 1;
	// basically add the numbers of the string to a unsigned long long untill there's an overflow
	// then allocates a new unsigned long long and adds from there
	while (string_len) {
		add = mult10 * ((s[string_len - 1]) - 48);
		// if there has been an overflow
		if (acc + add >= BASE || has_overflown(add)) {
			// reallocate the nums array (add one)
			(new -> len)++;
			temp = realloc(new_nums, (new -> len) * sizeof(Chunk));
			if (!temp) {
				errno = EFAULT;
				ALLOCATION_ERR;
				exit(1);
			}
			new_nums = temp;
			
			// add the number before the overflow (prec) to the nums array
			new_nums[(new -> len) - 1] = acc;

			// mult is 1
			mult10 = 1;
			// reset accumulator
			acc = 0;
			// new add
			add = (s[string_len - 1]) - 48;
		}

		acc += add;
		mult10 *= 10;
				
		string_len--;
	}

	if (acc || (new -> len == 0)) {
		(new -> len)++;
		new_nums = realloc(new_nums, (new -> len) * sizeof(Chunk));
		new_nums[(new -> len) - 1] = acc;
	}

	new -> nums = new_nums;
	
	return new;
}


void print_big_int(BigInt n) {
	if (!n) {
		printf("0\n");
		return;
	}
	int i;
	for (i = n -> len; i; i--)
		printf("%llu", (n -> nums)[i - 1]);
	printf("\n");
}

void print_big_int2(BigInt n) {
	int i;
	for (i = 0; i < n -> len; i++)
		printf("%llu\n", (n -> nums)[i]);
		       
}

char* big_int_to_string(BigInt n) {

	const size_t bound = (int) log10(BASE);

	int i;
	char* final = malloc((n -> len) * bound * sizeof(char));
	char num[bound + 1];

	
	final[0] = 0;

	
	for (i = n -> len; i; i--) {
		snprintf(num, bound + 1, "%llu", (n -> nums)[i - 1]);
		strcat(final, num);
	}

	return final;
}

int is_zero_big_int(BigInt n) {
	return ((n -> len) == 1) && ((n -> nums)[0] == 0);
}

BigInt add_big_int(BigInt a, BigInt b) {
	BigInt bigger  = a -> len > b -> len ? a : b;
	BigInt smaller = a -> len > b -> len ? b : a;
	Chunk big_value, small_value, rem, res;
	int big_index, small_index;

	BigInt result = malloc(sizeof(struct bigInt));

	result -> len = bigger -> len;
	result -> nums = malloc((result -> len) * sizeof(Chunk));
	
	for (big_index = small_index = rem = 0; big_index < bigger -> len; big_index++, small_index++) {
		small_value = small_index < smaller -> len ? (smaller -> nums)[small_index] : 0;
		big_value   = (bigger -> nums)[big_index];

		res = rem + big_value + small_value;

		if (res > BASE) {
			res -= BASE;
			rem = 1;
		} else rem = 0;
		
		(result -> nums)[big_index] = res;
	}
	
	if (rem) {
		(result -> len)++;
		// (result -> nums) = realloc(result -> nums, (result -> len) * sizeof(Chunk));

		(result -> nums)[result -> len - 1] = 1;
	}

	return result;
	
}

BigInt multiply_big_int_small_int(BigInt a, Chunk n) {
	if (n >= BASE) {
		return NULL;
	}

	int index;
	BigInt result = malloc(sizeof(struct bigInt));
	if (!result) {
		errno = EFAULT;
		ALLOCATION_ERR;
		exit(1);
	}

	Chunk rem, res, *temp;

	result -> len = (a -> len) + 1;
	result -> nums = malloc((result -> len) * sizeof(Chunk));
	
	for (index = rem = 0; index < a -> len; index++) {
		res = (n * (a -> nums)[index]) + rem;
		rem = res / BASE;
		res %= BASE;
		(result -> nums)[index] = res;
	}
	
	if (rem)
		(result -> nums)[index] = rem;
	else {
		
		temp = realloc(result -> nums, ((result -> len) - 1) * sizeof(Chunk));
		if (!temp) {
			errno = EFAULT;
			ALLOCATION_ERR;
			exit(1);
		} else {
			result -> nums = temp;
			(result -> len)--;
		}
	}

	return result;
}	

BigInt multiply_big_int(BigInt a, BigInt b) {
	if (!a || !b) {
		return NULL;
	}

	BigInt result = malloc(sizeof(struct bigInt));
	if (!result) {
		errno = EFAULT;
		ALLOCATION_ERR;
		exit(1);
	}

	int i, j;
	
	result -> len = (a -> len) + (b -> len);
	result -> nums = malloc((result -> len) * sizeof(Chunk));

	/* for (i = 0; i < a -> len; i++)  */
	/* 	for (j = rem = 0; j < b -> len || rem; j++) { */
	/* 		res = (a -> nums)[i] * (j < b -> len ? (b -> nums)[j] : 0) + rem; */
	/* 		(result -> nums)[i+j] = res % BASE; */
	/* 		rem = res / BASE; */
	/* 	} */

	BigInt res, new_res;
	
	for (i = 0; i < b -> len; i++) {
		res = multiply_big_int_small_int(a, (b -> nums)[i]);
		res -> nums = realloc(res -> nums, ((res -> len) + i) * sizeof(Chunk));
		res -> len += i;

		for (j = res -> len; j >= (res -> len) - i; j--)
			(res -> nums)[j - 1] = 0;

		new_res = add_big_int(result, res);
		destroy_big_int(res);
		result = new_res;
		destroy_big_int(result);
	}

	for (i = result -> len; i && !((result -> nums)[i - 1]); i--);
	
	if (i != (result -> len)) {
		result -> nums = realloc(result -> nums, i * sizeof(Chunk));
		result -> len = i;
	}
	

	
	return result;
}

void destroy_big_int(BigInt n) {
	free(n -> nums);
	free(n);
}
