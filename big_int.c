#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#define NDEBUG
#include <assert.h>

#define DEFAULT_BASE 	(unsigned long long) 10000000

#define CHECK_PTR_ALLOC(PTR, MSG, TERMINATING)			\
	if ((PTR) == NULL) {				\
		errno = EFAULT;			\
                        fprintf(stderr, "%s in `%s` at %s:%d\n",	\
		        (MSG), __func__, __FILE__, __LINE__);	\
		if (TERMINATING) exit(1);		\
	}						

#define DBG_PRINT_VALUE(VAL)								\
	#if !defined NDEBUG								\
		_Generic((VAL), 							\
		long long unsigned: printf("%llu in `%s` at %s:%d\n", (VAL), __func__, __FILE__, __LINE__),	\
		long unsigned: printf("%lu in `%s` at %s:%d\n",       (VAL), __func__, __FILE__, __LINE__),	\
		unsigned: printf("%u in `%s` at %s:%d\n",             (VAl), __func__, __FILE__, __LINE__),	\
		int: printf("%d in `%s` at %s:%d\n",                  (VAL), __func__, __FILE__, __LINE__),     \
		char: printf("%c in `%s` at %s:%d\n",                 (VAL), __func__, __FILE__, __LINE__),     \
		char*: printf("%s in `%s` at %s:%d\n",                (VAL), __func__, __FILE__, __LINE__),	\
		default: printf("%p in `%p` at %s:%d\n",              (VAL), __func__, __FILE__, __LINE__), 	\
		         ) (VAL)							\
           #endif

#define INIT_FMT_STRING(dest, num)	sprintf((dest), "%%0%dllu", (num));

typedef unsigned long long Chunk;

static Chunk base = DEFAULT_BASE;
static Chunk base_len = (int) log10(DEFAULT_BASE);

// len is the total number of chunks
// there is no sign for now
// the smallest digit is on the right
// using unsigned long long ints the base is 10^19
// so each number represented by a BigInt is
// (nums[0]) * base^0 + (nums[1]) * base^1 + ... + (nums[i]) * base^i + ... + (nums[n]) * base^n
// where n is len - 1.

typedef struct bigInt {
	Chunk* nums;
	size_t len;
}* BigInt;

static int has_overflown(Chunk mult);
void print_big_int2(BigInt n);
int is_zero_big_int(BigInt n);
BigInt new_big_int(char* s);
BigInt multiply_big_int_small_int(BigInt a, Chunk n);
void print_big_int(BigInt n);
BigInt multiply_big_int(BigInt a, BigInt b);
char* big_int_to_string(BigInt n);
void destroy_big_int(BigInt n);
int big_int_change_def_base(Chunk);


// better check for overflows
static int has_overflown(Chunk mult) {
	int res = 0;

	while (mult && (res < 2)) {
		res += (mult % 10) != 0;
		mult /= 10;
	}

	return res == 2;
}


// On input "1020303004875647366210" returns the big_int "10203034875647366210"
BigInt new_big_int(char* s) {
	if (s == NULL) return NULL;
            
	Chunk acc, mult10, add, i, *new_nums = NULL, *temp, string_len;

	// length of the string without leading zeroes
	for (string_len = 0; s[string_len]; string_len++);
	for (; s[string_len] == '0'; string_len--);
	
	BigInt new = malloc(sizeof(struct bigInt));
	CHECK_PTR_ALLOC(new, "allocation error (tried to `malloc` `new` but got `NULL`)", 1)
	
	new -> len = 0;

	// basically add the numbers of the string to a unsigned long long untill there's an overflow
	// then allocates a new unsigned long long and adds from there
	while (string_len) {
		// if there has been an overflow
		#ifndef NDEBUG
		printf("add: %llu\n", add);
		#endif
		for (i = 0, mult10 = 1, acc = 0;
		     i <= base_len && string_len;
		     i++, string_len--, acc += add, mult10 *= 10)
			add = mult10 * ((s[string_len - 1]) - 48);
		
		// reallocate the nums array (add one)
		(new -> len)++;
		new_nums = realloc(new_nums, (new -> len) * sizeof(Chunk));
		CHECK_PTR_ALLOC( new_nums
			   , "allocation error (tried to `realloc` `new_nums` but got `NULL`)"
			   , 1)
		
		// add the accumulator to the new_nums array
		new_nums[(new -> len) - 1] = acc;
		
                        #ifndef NDEBUG
		printf("%llu\n", acc);
		#endif
			
		// new add
		add = (s[string_len - 1]) - 48;
	}

	new -> nums = new_nums;
	
	return new;
}


void print_big_int(BigInt n) {
	CHECK_PTR_ALLOC(n, "allocation error", 1)
	if (!n) {
		printf("0\n");
		return;
	}

	
	int i;
	char fmt[20];
	INIT_FMT_STRING(fmt, base_len)
	printf("%llu", (n -> nums)[n -> len - 1]);
	for (i = n -> len - 1; i; i--)
		printf(fmt, (n -> nums)[i - 1]);
	printf("\n");
}

void print_big_int2(BigInt n) {
	CHECK_PTR_ALLOC(n, "allocation error (the argument cannot be `NULL`)", 0);
	if (n == NULL) return;
	
	int i;
	for (i = 0; i < n -> len; i++)
		printf("%llu\n", (n -> nums)[i]);
		       
}

char* big_int_to_string(BigInt n) {

	const size_t bound = (int) log10(base);

	int i;
	char* final = malloc((n -> len) * bound * sizeof(char));
	CHECK_PTR_ALLOC(final, "allocation error", 1)
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

		if (res > base) {
			res -= base;
			rem = 1;
		} else rem = 0;
		
		(result -> nums)[big_index] = res;
	}
	
	if (rem) {
		(result -> len)++;
		(result -> nums) = realloc(result -> nums, (result -> len) * sizeof(Chunk));
		CHECK_PTR_ALLOC(result -> nums, "allocation error", 1)
		(result -> nums)[result -> len - 1] = 1;
	}

	return result;
	
}

BigInt multiply_big_int_small_int(BigInt a, Chunk n) {
	if (n >= base) {
		return NULL;
	}

	int index;
	BigInt result = malloc(sizeof(struct bigInt));
	CHECK_PTR_ALLOC(result, "allocation error", 1)
		
	Chunk rem, res, *temp;

	result -> len = (a -> len) + 1;
	result -> nums = malloc((result -> len) * sizeof(Chunk));
	
	for (index = rem = 0; index < a -> len; index++) {
		res = (n * (a -> nums)[index]) + rem;
		rem = res / base;
		res %= base;
		(result -> nums)[index] = res;
	}
	
	if (rem)
		(result -> nums)[index] = rem;
	else {
		
		temp = realloc(result -> nums, ((result -> len) - 1) * sizeof(Chunk));
		CHECK_PTR_ALLOC(temp, "allocation error (tried to `realloc` `temp` but got `NULL`)", 1)
		result -> nums = temp;
		(result -> len)--;
	}

	return result;
}	

// TODO: segfaults
BigInt multiply_big_int(BigInt a, BigInt b) {
	// a nor b can be NULL
	if (!a || !b) {
		return NULL;
	}

	
	BigInt result = malloc(sizeof(struct bigInt));
	CHECK_PTR_ALLOC(result, "allocation error (tried do `malloc` `result` but got `NULL`)", 1)

	int i, j;
	
	result -> len = (a -> len) + (b -> len);
	result -> nums = malloc((result -> len) * sizeof(Chunk));

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

	// shorten the bignum eliminating leading zeroes
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

int big_int_change_def_base(Chunk n) {
	// the base cannot be 0 or 1
	if (n == 0 || n == 1) return 0;
	
	base = n;
	base_len = (int) log10(n);
	return 1;
}
