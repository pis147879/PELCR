/* Optional arbitrary-precision user values.

 Copyright (C) 1997-2015 Marco Pedicini

 This file is part of PELCR. */

#ifndef PELCR_VALUE_H
#define PELCR_VALUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAXUSERVALUELEN
#define MAXUSERVALUELEN 256
#endif

#ifdef PELCR_GMP_USERTYPE
#include <gmp.h>

typedef struct pelcr_value {
	mpz_t z;
} USERTYPE;

static inline void
pelcr_value_init(USERTYPE *v) {
	mpz_init(v->z);
}

static inline void
pelcr_value_clear(USERTYPE *v) {
	mpz_clear(v->z);
}

static inline void
pelcr_value_set(USERTYPE *dst, const USERTYPE *src) {
	mpz_set(dst->z, src->z);
}

static inline void
pelcr_value_set_si(USERTYPE *dst, long long n) {
	mpz_set_si(dst->z, n);
}

static inline int
pelcr_value_set_str(USERTYPE *dst, const char *s) {
	return mpz_set_str(dst->z, s, 0);
}

static inline USERTYPE
pelcr_value_from_si(long long n) {
	USERTYPE v;
	pelcr_value_init(&v);
	pelcr_value_set_si(&v, n);
	return v;
}

static inline USERTYPE
pelcr_value_from_str(const char *s) {
	USERTYPE v;
	pelcr_value_init(&v);
	if (pelcr_value_set_str(&v, s) != 0)
		pelcr_value_set_si(&v, 0);
	return v;
}

static inline USERTYPE
pelcr_value_copy(const USERTYPE *src) {
	USERTYPE v;
	pelcr_value_init(&v);
	pelcr_value_set(&v, src);
	return v;
}

static inline int
pelcr_value_is_zero(const USERTYPE *v) {
	return mpz_sgn(v->z) == 0;
}

static inline int
pelcr_value_cmp(const USERTYPE *a, const USERTYPE *b) {
	return mpz_cmp(a->z, b->z);
}

static inline long long
pelcr_value_get_ll(const USERTYPE *v) {
	return mpz_get_si(v->z);
}

static inline void
pelcr_value_to_str(const USERTYPE *v, char *buf, size_t buflen) {
	char *tmp;
	size_t len;
	void (*freefunc)(void *, size_t);

	if (buflen == 0)
		return;
	tmp = mpz_get_str(NULL, 10, v->z);
	len = strlen(tmp);
	if (len >= buflen) {
		fprintf(stderr, "PELCR: GMP user value is too large for MPI message buffer (%zu >= %zu)\n", len, buflen);
		exit(1);
	}
	memcpy(buf, tmp, len + 1);
	mp_get_memory_functions(NULL, NULL, &freefunc);
	freefunc(tmp, len + 1);
}

static inline void
pelcr_value_print(FILE *stream, const USERTYPE *v) {
	mpz_out_str(stream, 10, v->z);
}

static inline USERTYPE
pelcr_value_add(const USERTYPE *a, const USERTYPE *b) {
	USERTYPE r;
	pelcr_value_init(&r);
	mpz_add(r.z, a->z, b->z);
	return r;
}

static inline USERTYPE
pelcr_value_sub_ui(const USERTYPE *a, unsigned long b) {
	USERTYPE r;
	pelcr_value_init(&r);
	mpz_sub_ui(r.z, a->z, b);
	return r;
}

static inline USERTYPE
pelcr_value_add_ui(const USERTYPE *a, unsigned long b) {
	USERTYPE r;
	pelcr_value_init(&r);
	mpz_add_ui(r.z, a->z, b);
	return r;
}

static inline USERTYPE
pelcr_value_mul(const USERTYPE *a, const USERTYPE *b) {
	USERTYPE r;
	pelcr_value_init(&r);
	mpz_mul(r.z, a->z, b->z);
	return r;
}

#else

#ifndef USERTYPE
#define USERTYPE long long
#endif

static inline void
pelcr_value_init(USERTYPE *v) {
	*v = 0;
}

static inline void
pelcr_value_clear(USERTYPE *v) {
	(void)v;
}

static inline void
pelcr_value_set(USERTYPE *dst, const USERTYPE *src) {
	*dst = *src;
}

static inline void
pelcr_value_set_si(USERTYPE *dst, long long n) {
	*dst = (USERTYPE)n;
}

static inline int
pelcr_value_set_str(USERTYPE *dst, const char *s) {
	char *end = NULL;
	*dst = (USERTYPE)strtoll(s, &end, 0);
	return (end == s) ? -1 : 0;
}

static inline USERTYPE
pelcr_value_from_si(long long n) {
	return (USERTYPE)n;
}

static inline USERTYPE
pelcr_value_from_str(const char *s) {
	USERTYPE v;
	pelcr_value_set_str(&v, s);
	return v;
}

static inline USERTYPE
pelcr_value_copy(const USERTYPE *src) {
	return *src;
}

static inline int
pelcr_value_is_zero(const USERTYPE *v) {
	return *v == 0;
}

static inline int
pelcr_value_cmp(const USERTYPE *a, const USERTYPE *b) {
	return (*a > *b) - (*a < *b);
}

static inline long long
pelcr_value_get_ll(const USERTYPE *v) {
	return (long long)*v;
}

static inline void
pelcr_value_to_str(const USERTYPE *v, char *buf, size_t buflen) {
	snprintf(buf, buflen, "%lld", (long long)*v);
}

static inline void
pelcr_value_print(FILE *stream, const USERTYPE *v) {
	fprintf(stream, "%lld", (long long)*v);
}

static inline USERTYPE
pelcr_value_add(const USERTYPE *a, const USERTYPE *b) {
	return (USERTYPE)(*a + *b);
}

static inline USERTYPE
pelcr_value_sub_ui(const USERTYPE *a, unsigned long b) {
	return (USERTYPE)(*a - (USERTYPE)b);
}

static inline USERTYPE
pelcr_value_add_ui(const USERTYPE *a, unsigned long b) {
	return (USERTYPE)(*a + (USERTYPE)b);
}

static inline USERTYPE
pelcr_value_mul(const USERTYPE *a, const USERTYPE *b) {
	return (USERTYPE)(*a * *b);
}

#endif

#ifdef PELCR_GMP_USERTYPE
typedef char PELCR_WIRE_VALUE[MAXUSERVALUELEN];
#define pelcr_wire_clear(wire) ((wire)[0] = '\0')
#define pelcr_value_to_wire(value, wire) pelcr_value_to_str((value), (wire), sizeof(wire))
#define pelcr_value_from_wire(value, wire) pelcr_value_set_str((value), (wire))
#else
typedef USERTYPE PELCR_WIRE_VALUE;
#define pelcr_wire_clear(wire) ((wire) = 0)
#define pelcr_value_to_wire(value, wire) ((wire) = *(value))
#define pelcr_value_from_wire(value, wire) (pelcr_value_set((value), &(wire)), 0)
#endif

#endif
