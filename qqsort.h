/*
 * SPDX-FileCopyrightText: Copyright 2023 Denis Glazkov <glazzk.off@mail.ru>
 * SPDX-License-Identifier: MIT
 *
 * Reimplementation of qsort function from glibc with
 * more ability to inline comparison function.
 *
 * https://codebrowser.dev/glibc/glibc/stdlib/qsort.c.html
 */

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define __QQSORT_SWAP(A, B, N) \
    do {                       \
        uint8_t temp;          \
                               \
        uint8_t *a = (A);      \
        uint8_t *b = (B);      \
                               \
        size_t i = 0;          \
                               \
        while (i++ < (N)) {    \
            temp = *a;         \
            *a++ = *b;         \
            *b++ = temp;       \
        }                      \
    } while (0)

#define __QQSORT_STACK_PUSH(L, R, TOP) \
    do {                               \
        (TOP)->l = (L);                \
        (TOP)->r = (R);                \
        (TOP)++;                       \
    } while (0)

#define __QQSORT_STACK_POP(L, R, TOP) \
    do {                              \
        (TOP)--;                      \
        (L) = (TOP)->l;               \
        (R) = (TOP)->r;               \
    } while (0)

#define __QQSORT_CMP_RESULT_VARIABLE_NAME __qqsort_cmp_result
#define __QQSORT_CMP_A_VARIABLE_NAME      __qqsort_cmp_a
#define __QQSORT_CMP_B_VARIABLE_NAME      __qqsort_cmp_b

#define __QQSORT_CMP(A, B, CMP_CALLBACK, IF_CALLBACK, ELSE_CALLBACK) \
    do {                                                             \
        void *__QQSORT_CMP_A_VARIABLE_NAME = (void *) (A);           \
        void *__QQSORT_CMP_B_VARIABLE_NAME = (void *) (B);           \
                                                                     \
        int __QQSORT_CMP_RESULT_VARIABLE_NAME = 0;                   \
        CMP_CALLBACK                                                 \
                                                                     \
        if (__QQSORT_CMP_RESULT_VARIABLE_NAME < 0)                   \
            IF_CALLBACK                                              \
        else                                                         \
            ELSE_CALLBACK                                            \
    } while (0)

#define __QQSORT_INF_LOOP_FLAG __qqsort_infinite_loop_stop
#define __QQSORT_INF_LOOP_STOP __QQSORT_INF_LOOP_FLAG = true
#define __QQSORT_INF_LOOP      __QQSORT_INF_LOOP_FLAG = false; while (!__QQSORT_INF_LOOP_FLAG)

/**
 * Utility class to avoid the error of converting
 * void* to any other pointer when using C++.
 */
#ifdef __cplusplus
template <typename T>
struct __QQSORT_AUTOCAST
{
    constexpr __QQSORT_AUTOCAST(T *ptr) : ptr(ptr) {}

    template <typename U>
    constexpr operator U() { return static_cast<U>(ptr); }

    T *ptr;
};
#else
#define __QQSORT_AUTOCAST(PTR) (PTR)
#endif

#define qqsortret(A) __QQSORT_CMP_RESULT_VARIABLE_NAME = (A)
#define qqsortcmp(A, B)                                  \
    A = __QQSORT_AUTOCAST(__QQSORT_CMP_A_VARIABLE_NAME); \
    B = __QQSORT_AUTOCAST(__QQSORT_CMP_B_VARIABLE_NAME);

typedef struct {
    uint8_t *l;
    uint8_t *r;
} __qqsort_stack_node_t;

#define qqsort(ARR, LEN, SIZE, CMP)                                                                \
    do {                                                                                           \
        bool __QQSORT_INF_LOOP_FLAG;                                                               \
                                                                                                   \
        uint8_t *base = (uint8_t *) (ARR);                                                         \
        size_t maxthresh = 4 * (SIZE);                                                             \
                                                                                                   \
        if ((LEN) == 0) break;                                                                     \
                                                                                                   \
        if ((size_t) (LEN) > maxthresh) {                                                          \
            uint8_t *l = base;                                                                     \
            uint8_t *r = l + (SIZE) * ((LEN) - 1);                                                 \
                                                                                                   \
            __qqsort_stack_node_t stack[sizeof(size_t) * CHAR_BIT];                                \
            __qqsort_stack_node_t *top = stack;                                                    \
                                                                                                   \
            __QQSORT_STACK_PUSH(NULL, NULL, top);                                                  \
                                                                                                   \
            while (stack < top) {                                                                  \
                uint8_t *ll;                                                                       \
                uint8_t *rr;                                                                       \
                uint8_t *mm = l + (SIZE) * ((r - l) / (SIZE) >> 1);                                \
                                                                                                   \
                __QQSORT_CMP(mm, l, CMP, { __QQSORT_SWAP(mm, l, (SIZE)); }, {});                   \
                __QQSORT_CMP(r, mm, CMP,                                                           \
                             {                                                                     \
                                 __QQSORT_SWAP(mm, r, (SIZE));                                     \
                                 __QQSORT_CMP(mm, l, CMP, { __QQSORT_SWAP(mm, l, (SIZE)); }, {});  \
                             },                                                                    \
                             {});                                                                  \
                                                                                                   \
                ll = l + (SIZE);                                                                   \
                rr = r - (SIZE);                                                                   \
                                                                                                   \
                do {                                                                               \
                    __QQSORT_INF_LOOP {                                                            \
                        __QQSORT_CMP(ll, mm, CMP, { ll += (SIZE); }, { __QQSORT_INF_LOOP_STOP; }); \
                    }                                                                              \
                                                                                                   \
                    __QQSORT_INF_LOOP {                                                            \
                        __QQSORT_CMP(mm, rr, CMP, { rr -= (SIZE); }, { __QQSORT_INF_LOOP_STOP; }); \
                    }                                                                              \
                                                                                                   \
                    if (ll < rr) {                                                                 \
                        __QQSORT_SWAP(ll, rr, (SIZE));                                             \
                                                                                                   \
                        if (mm == ll) {                                                            \
                            mm = rr;                                                               \
                        } else if (mm == rr) {                                                     \
                            mm = ll;                                                               \
                        }                                                                          \
                                                                                                   \
                        ll += (SIZE);                                                              \
                        rr -= (SIZE);                                                              \
                    } else if (ll == rr) {                                                         \
                        ll += (SIZE);                                                              \
                        rr -= (SIZE);                                                              \
                        break;                                                                     \
                    }                                                                              \
                } while (ll <= rr);                                                                \
                                                                                                   \
                if ((size_t) (rr - l) <= maxthresh) {                                              \
                    if ((size_t) (r - ll) <= maxthresh) {                                          \
                        __QQSORT_STACK_POP(l, r, top);                                             \
                    } else {                                                                       \
                        l = ll;                                                                    \
                    }                                                                              \
                } else if ((size_t) (r - ll) <= maxthresh) {                                       \
                    r = rr;                                                                        \
                } else if ((rr - l) > (r - ll)) {                                                  \
                    __QQSORT_STACK_PUSH(l, rr, top);                                               \
                    l = ll;                                                                        \
                } else {                                                                           \
                    __QQSORT_STACK_PUSH(ll, r, top);                                               \
                    r = rr;                                                                        \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
                                                                                                   \
        {                                                                                          \
            uint8_t *end = base + (SIZE) * ((LEN) -1);                                             \
            uint8_t *tmp = base;                                                                   \
            uint8_t *thresh = end;                                                                 \
            uint8_t *run = tmp + (SIZE);                                                           \
                                                                                                   \
            if (thresh < base + maxthresh) {                                                       \
                thresh = base + maxthresh;                                                         \
            }                                                                                      \
                                                                                                   \
            while (run <= thresh) {                                                                \
                __QQSORT_CMP(run, tmp, CMP, { tmp = run; }, {});                                   \
                run += (SIZE);                                                                     \
            }                                                                                      \
                                                                                                   \
            if (tmp != base) {                                                                     \
                __QQSORT_SWAP(tmp, base, (SIZE));                                                  \
            }                                                                                      \
                                                                                                   \
            run = base + (SIZE);                                                                   \
                                                                                                   \
            while ((run += (SIZE)) <= end) {                                                       \
                tmp = run - (SIZE);                                                                \
                                                                                                   \
                __QQSORT_INF_LOOP {                                                                \
                    __QQSORT_CMP(run, tmp, CMP, { tmp -= (SIZE); }, { __QQSORT_INF_LOOP_STOP; });  \
                }                                                                                  \
                                                                                                   \
                tmp += (SIZE);                                                                     \
                                                                                                   \
                if (tmp != run) {                                                                  \
                    uint8_t *trav = run + (SIZE);                                                  \
                                                                                                   \
                    while (--trav >= run) {                                                        \
                        uint8_t *h = trav;                                                         \
                        uint8_t *l = trav;                                                         \
                                                                                                   \
                        uint8_t c = *trav;                                                         \
                                                                                                   \
                        while ((l -= (SIZE)) >= tmp) {                                             \
                            *h = *l;                                                               \
                             h =  l;                                                               \
                        }                                                                          \
                                                                                                   \
                        *h = c;                                                                    \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    } while (0)
