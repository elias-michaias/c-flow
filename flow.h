#ifndef FLOW_H
#define FLOW_H

#ifndef PIPE_PLACEHOLDER
#define PIPE_PLACEHOLDER _
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Block.h>

typedef struct {
    void *data;
    size_t len;
    size_t elem_size;
} Iterator;

#define to_iter(arr) \
    ((Iterator){ .data = (arr), .len = sizeof(arr)/sizeof(*(arr)), .elem_size = sizeof(*(arr)) })

// iter_map: map each element with an expression (in_type, x, out_type, expr)
#define iter_map(iter, in_type, x, out_type, expr) \
    ({ \
        Iterator _it = (iter); \
        out_type *_out = malloc(_it.len * sizeof(out_type)); \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            in_type x = ((in_type*)_it.data)[_i]; \
            _out[_i] = (expr); \
        } \
        (Iterator){ .data = _out, .len = _it.len, .elem_size = sizeof(out_type) }; \
    })

// iter_then: run an arbitrary statement/expression in the middle of a pipeline, returns the iterator unchanged
#define iter_then(iter, expr) \
    ({ \
        Iterator _it = (iter); \
        expr; \
        _it; \
    })

// iter_slice: return a subrange [start, end) of the iterator
#define iter_slice(iter, start, end) \
    ({ \
        Iterator _it = (iter); \
        size_t _s = (start) < _it.len ? (start) : _it.len; \
        size_t _e = (end) < _it.len ? (end) : _it.len; \
        (Iterator){ .data = (char*)_it.data + _s * _it.elem_size, .len = (_e > _s ? _e - _s : 0), .elem_size = _it.elem_size }; \
    })

// iter_filter: filter elements by predicate (type, x is the element)
#define iter_filter(iter, type, x, pred) \
    ({ \
        Iterator _it = (iter); \
        type *_out = malloc(_it.len * sizeof(type)); \
        size_t _count = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            if (pred) _out[_count++] = x; \
        } \
        (Iterator){ .data = _out, .len = _count, .elem_size = sizeof(type) }; \
    })

// iter_sum: sum all elements (assumes numeric type)
#define iter_sum(iter, type) \
    ({ \
        Iterator _it = (iter); \
        type _sum = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) _sum += ((type*)_it.data)[_i]; \
        _sum; \
    })

// iter_for: perform an operation on each element (type, x is the element), return the iterator
#define iter_for(iter, type, x, op) \
    ({ \
        Iterator _it = (iter); \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            op; \
        } \
        _it; \
    })

// iter_take: take the first n elements from an iterator
#define iter_take(iter, n) \
    ({ \
        Iterator _it = (iter); \
        size_t _n = (n) < _it.len ? (n) : _it.len; \
        (Iterator){ .data = _it.data, .len = _n, .elem_size = _it.elem_size }; \
    })

// iter_drop: drop the first n elements from an iterator
#define iter_drop(iter, n) \
    ({ \
        Iterator _it = (iter); \
        size_t _n = (n) < _it.len ? (n) : _it.len; \
        (Iterator){ .data = (char*)_it.data + _n * _it.elem_size, .len = _it.len - _n, .elem_size = _it.elem_size }; \
    })

// iter_reverse: return a new iterator with elements in reverse order
#define iter_reverse(iter) \
    ({ \
        Iterator _it = (iter); \
        void* _out = malloc(_it.len * _it.elem_size); \
        for (size_t _i = 0; _i < _it.len; ++_i) \
            memcpy((char*)_out + _i * _it.elem_size, \
                   (char*)_it.data + (_it.len - 1 - _i) * _it.elem_size, \
                   _it.elem_size); \
        (Iterator){ .data = _out, .len = _it.len, .elem_size = _it.elem_size }; \
    })

// iter_unique: return only the first occurrence of each element (memcmp version)
#define iter_unique(iter) \
    ({ \
        Iterator _it = (iter); \
        void* _out = malloc(_it.len * _it.elem_size); \
        size_t _count = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            int _found = 0; \
            for (size_t _j = 0; _j < _count; ++_j) \
                if (memcmp((char*)_it.data + _i * _it.elem_size, (char*)_out + _j * _it.elem_size, _it.elem_size) == 0) { _found = 1; break; } \
            if (!_found) \
                memcpy((char*)_out + _count++ * _it.elem_size, (char*)_it.data + _i * _it.elem_size, _it.elem_size); \
        } \
        (Iterator){ .data = _out, .len = _count, .elem_size = _it.elem_size }; \
    })

// iter_concat: concatenate two iterators of the same type
#define iter_concat(iter1, iter2) \
    ({ \
        Iterator _a = (iter1), _b = (iter2); \
        void* _out = malloc((_a.len + _b.len) * _a.elem_size); \
        memcpy(_out, _a.data, _a.len * _a.elem_size); \
        memcpy((char*)_out + _a.len * _a.elem_size, _b.data, _b.len * _b.elem_size); \
        (Iterator){ .data = _out, .len = _a.len + _b.len, .elem_size = _a.elem_size }; \
    })

// Internal, pointer-based version (do not use directly)
#define _iter_pad_ptr(iter, newlen, padptr) \
    ({ \
        Iterator _it = (iter); \
        size_t _n = (newlen); \
        void* _out = malloc(_n * _it.elem_size); \
        size_t _i = 0; \
        for (; _i < _it.len && _i < _n; ++_i) \
            memcpy((char*)_out + _i * _it.elem_size, (char*)_it.data + _i * _it.elem_size, _it.elem_size); \
        for (; _i < _n; ++_i) \
            memcpy((char*)_out + _i * _it.elem_size, (padptr), _it.elem_size); \
        (Iterator){ .data = _out, .len = _n, .elem_size = _it.elem_size }; \
    })
// User-facing, value-based version (for scalars)
#define iter_pad(iter, newlen, padval) \
    ({ \
        __auto_type _padval = (padval); \
        _iter_pad_ptr((iter), (newlen), &_padval); \
    })

// iter_repeat: repeat the iterator's elements a given number of times
#define iter_repeat(iter, times) \
    ({ \
        Iterator _it = (iter); \
        size_t _t = (times); \
        void* _out = malloc(_it.len * _t * _it.elem_size); \
        for (size_t _i = 0; _i < _t; ++_i) \
            memcpy((char*)_out + _i * _it.len * _it.elem_size, _it.data, _it.len * _it.elem_size); \
        (Iterator){ .data = _out, .len = _it.len * _t, .elem_size = _it.elem_size }; \
    })

// iter_foldl: left fold (accumulate from left to right)
// Usage: iter_foldl(iter, type, acc_type, acc, x, init, expr)
#define iter_foldl(iter, type, acc_type, acc, x, init, expr) \
    ({ \
        Iterator _it = (iter); \
        acc_type acc = (init); \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            acc = (expr); \
        } \
        acc; \
    })

// iter_foldr: right fold (accumulate from right to left)
// Usage: iter_foldr(iter, type, acc_type, acc, x, init, expr)
#define iter_foldr(iter, type, acc_type, acc, x, init, expr) \
    ({ \
        Iterator _it = (iter); \
        acc_type acc = (init); \
        for (ptrdiff_t _i = (ptrdiff_t)_it.len - 1; _i >= 0; --_i) { \
            type x = ((type*)_it.data)[_i]; \
            acc = (expr); \
        } \
        acc; \
    })
    
// iter_zip: zip two iterators into an iterator of pairtype (fields .a and .b)
// Usage: iter_zip(it1, it1type, it2, it2type, pairtype)
#define iter_zip(it1type, it1, it2type, it2, pairtype) \
    ({ \
        Iterator _a = (it1), _b = (it2); \
        size_t _n = _a.len < _b.len ? _a.len : _b.len; \
        pairtype* _out = malloc(_n * sizeof(pairtype)); \
        for (size_t _i = 0; _i < _n; ++_i) { \
            _out[_i] = (pairtype){ .a = ((it1type*)_a.data)[_i], .b = ((it2type*)_b.data)[_i] }; \
        } \
        (Iterator){ .data = _out, .len = _n, .elem_size = sizeof(pairtype) }; \
    })

// iter_flatten: flatten an iterator of iterators (assumes all inner iterators have same type)
#define iter_flatten(iter, itertype, elemtype) \
    ({ \
        Iterator _it = (iter); \
        size_t _total = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            itertype inner = ((itertype*)_it.data)[_i]; \
            _total += inner.len; \
        } \
        elemtype* _out = malloc(_total * sizeof(elemtype)); \
        size_t _pos = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            itertype inner = ((itertype*)_it.data)[_i]; \
            for (size_t _j = 0; _j < inner.len; ++_j) \
                _out[_pos++] = ((elemtype*)inner.data)[_j]; \
        } \
        (Iterator){ .data = _out, .len = _total, .elem_size = sizeof(elemtype) }; \
    })

// iter_partition: split an iterator into two by predicate (returns a struct with .yes and .no fields)
typedef struct { Iterator yes, no; } IteratorPartitionResult;
#define iter_partition(iter, type, x, pred) \
    ({ \
        Iterator _it = (iter); \
        type* _yes = malloc(_it.len * sizeof(type)); \
        type* _no = malloc(_it.len * sizeof(type)); \
        size_t _y = 0, _n = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            if (pred) _yes[_y++] = x; \
            else _no[_n++] = x; \
        } \
        (IteratorPartitionResult){ \
            .yes = (Iterator){ .data = _yes, .len = _y, .elem_size = sizeof(type) }, \
            .no = (Iterator){ .data = _no, .len = _n, .elem_size = sizeof(type) } \
        }; \
    })

// iter_scan: prefix sum (or any scan operation), simplified
#define iter_scan(iter, type, x, init, expr) \
    ({ \
        Iterator _it = (iter); \
        type acc = (init); \
        type* _out = malloc(_it.len * sizeof(type)); \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            acc = (expr); \
            _out[_i] = acc; \
        } \
        (Iterator){ .data = _out, .len = _it.len, .elem_size = sizeof(type) }; \
    })

// iter_any: returns 1 if any element matches predicate
#define iter_any(iter, type, x, pred) \
    ({ \
        Iterator _it = (iter); \
        int _found = 0; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            if (pred) { _found = 1; break; } \
        } \
        _found; \
    })

// iter_all: returns 1 if all elements match predicate
#define iter_all(iter, type, x, pred) \
    ({ \
        Iterator _it = (iter); \
        int _all = 1; \
        for (size_t _i = 0; _i < _it.len; ++_i) { \
            type x = ((type*)_it.data)[_i]; \
            if (!(pred)) { _all = 0; break; } \
        } \
        _all; \
    })

// iter_range: create an iterator over a range [start, end) (step=1)
#define iter_range(type, start, end) \
    ({ \
        type _s = (start), _e = (end); \
        size_t _n = (_e > _s) ? (_e - _s) : 0; \
        type* _out = malloc(_n * sizeof(type)); \
        for (size_t _i = 0; _i < _n; ++_i) _out[_i] = _s + (type)_i; \
        (Iterator){ .data = _out, .len = _n, .elem_size = sizeof(type) }; \
    })

// Pipe macros (as before)
#define PIPE_STEP_1(init, s1) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER; })
#define PIPE_STEP_2(init, s1, s2) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER; })
#define PIPE_STEP_3(init, s1, s2, s3) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER; })
#define PIPE_STEP_4(init, s1, s2, s3, s4) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER; })
#define PIPE_STEP_5(init, s1, s2, s3, s4, s5) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER = (s5); PIPE_PLACEHOLDER; })
#define PIPE_STEP_6(init, s1, s2, s3, s4, s5, s6) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER = (s5); PIPE_PLACEHOLDER = (s6); PIPE_PLACEHOLDER; })
#define PIPE_STEP_7(init, s1, s2, s3, s4, s5, s6, s7) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER = (s5); PIPE_PLACEHOLDER = (s6); PIPE_PLACEHOLDER = (s7); PIPE_PLACEHOLDER; })
#define PIPE_STEP_8(init, s1, s2, s3, s4, s5, s6, s7, s8) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER = (s5); PIPE_PLACEHOLDER = (s6); PIPE_PLACEHOLDER = (s7); PIPE_PLACEHOLDER = (s8); PIPE_PLACEHOLDER; })
#define PIPE_STEP_9(init, s1, s2, s3, s4, s5, s6, s7, s8, s9) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER = (s5); PIPE_PLACEHOLDER = (s6); PIPE_PLACEHOLDER = (s7); PIPE_PLACEHOLDER = (s8); PIPE_PLACEHOLDER = (s9); PIPE_PLACEHOLDER; })
#define PIPE_STEP_10(init, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10) \
    ({ typeof(init) PIPE_PLACEHOLDER = (init); PIPE_PLACEHOLDER = (s1); PIPE_PLACEHOLDER = (s2); PIPE_PLACEHOLDER = (s3); PIPE_PLACEHOLDER = (s4); PIPE_PLACEHOLDER = (s5); PIPE_PLACEHOLDER = (s6); PIPE_PLACEHOLDER = (s7); PIPE_PLACEHOLDER = (s8); PIPE_PLACEHOLDER = (s9); PIPE_PLACEHOLDER = (s10); PIPE_PLACEHOLDER; })

#define GET_PIPE_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,NAME,...) NAME

#define pipe(...) \
    GET_PIPE_MACRO(__VA_ARGS__, \
        PIPE_STEP_10, PIPE_STEP_9, PIPE_STEP_8, PIPE_STEP_7, PIPE_STEP_6, \
        PIPE_STEP_5, PIPE_STEP_4, PIPE_STEP_3, PIPE_STEP_2, PIPE_STEP_1)(__VA_ARGS__)

// chain: direct unary function nesting, no placeholder support
#define CHAIN_2(input, f1)           f1(input)
#define CHAIN_3(input, f1, f2)       f2(f1(input))
#define CHAIN_4(input, f1, f2, f3, f4)   f4(f3(f2(f1(input))))
#define CHAIN_5(input, f1, f2, f3, f4) f4(f3(f2(f1(input))))
#define CHAIN_6(input, f1, f2, f3, f4, f5) f5(f4(f3(f2(f1(input)))))
#define CHAIN_7(input, f1, f2, f3, f4, f5, f6) f6(f5(f4(f3(f2(f1(input))))))
#define CHAIN_8(input, f1, f2, f3, f4, f5, f6, f7) f7(f6(f5(f4(f3(f2(f1(input)))))))
#define CHAIN_9(input, f1, f2, f3, f4, f5, f6, f7, f8) f8(f7(f6(f5(f4(f3(f2(f1(input))))))))
#define CHAIN_10(input, f1, f2, f3, f4, f5, f6, f7, f8, f9) f9(f8(f7(f6(f5(f4(f3(f2(f1(input)))))))))

#define GET_CHAIN_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,NAME,...) NAME
#define chain(...) \
    GET_CHAIN_MACRO(__VA_ARGS__, \
        CHAIN_10, CHAIN_9, CHAIN_8, CHAIN_7, CHAIN_6, \
        CHAIN_5, CHAIN_4, CHAIN_3, CHAIN_2)(__VA_ARGS__)


#ifdef __clang__
// Type-safe, ergonomic curry macro: curry(f, T1, T2, ..., TN)
// Usage: curry(add5, float, float, float, float, float)
#define _CURRY_BLOCK_TYPED_1(f, T1) \
    Block_copy(^(T1 a1) { return f(a1); })
#define _CURRY_BLOCK_TYPED_2(f, T1, T2) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return f(a1, a2); }); })
#define _CURRY_BLOCK_TYPED_3(f, T1, T2, T3) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return f(a1, a2, a3); }); }); })
#define _CURRY_BLOCK_TYPED_4(f, T1, T2, T3, T4) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return f(a1, a2, a3, a4); }); }); }); })
#define _CURRY_BLOCK_TYPED_5(f, T1, T2, T3, T4, T5) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return Block_copy(^(T5 a5) { return f(a1, a2, a3, a4, a5); }); }); }); }); })
#define _CURRY_BLOCK_TYPED_6(f, T1, T2, T3, T4, T5, T6) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return Block_copy(^(T5 a5) { return Block_copy(^(T6 a6) { return f(a1, a2, a3, a4, a5, a6); }); }); }); }); }); })
#define _CURRY_BLOCK_TYPED_7(f, T1, T2, T3, T4, T5, T6, T7) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return Block_copy(^(T5 a5) { return Block_copy(^(T6 a6) { return Block_copy(^(T7 a7) { return f(a1, a2, a3, a4, a5, a6, a7); }); }); }); }); }); }); })
#define _CURRY_BLOCK_TYPED_8(f, T1, T2, T3, T4, T5, T6, T7, T8) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return Block_copy(^(T5 a5) { return Block_copy(^(T6 a6) { return Block_copy(^(T7 a7) { return Block_copy(^(T8 a8) { return f(a1, a2, a3, a4, a5, a6, a7, a8); }); }); }); }); }); }); }); })
#define _CURRY_BLOCK_TYPED_9(f, T1, T2, T3, T4, T5, T6, T7, T8, T9) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return Block_copy(^(T5 a5) { return Block_copy(^(T6 a6) { return Block_copy(^(T7 a7) { return Block_copy(^(T8 a8) { return Block_copy(^(T9 a9) { return f(a1, a2, a3, a4, a5, a6, a7, a8, a9); }); }); }); }); }); }); }); }); })
#define _CURRY_BLOCK_TYPED_10(f, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) \
    Block_copy(^(T1 a1) { return Block_copy(^(T2 a2) { return Block_copy(^(T3 a3) { return Block_copy(^(T4 a4) { return Block_copy(^(T5 a5) { return Block_copy(^(T6 a6) { return Block_copy(^(T7 a7) { return Block_copy(^(T8 a8) { return Block_copy(^(T9 a9) { return Block_copy(^(T10 a10) { return f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }); }); }); }); }); }); }); }); }); })
#define _CURRY_TYPED_NARGS(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define _CURRY_TYPED_DISPATCH_INDIRECT(f, N, ...) _CURRY_BLOCK_TYPED_##N(f, __VA_ARGS__)
#define _CURRY_TYPED_DISPATCH(f, N, ...) _CURRY_TYPED_DISPATCH_INDIRECT(f, N, __VA_ARGS__)
#define curry(f, ...) _CURRY_TYPED_DISPATCH(f, _CURRY_TYPED_NARGS(_, ##__VA_ARGS__,10,9,8,7,6,5,4,3,2,1), __VA_ARGS__)
#endif 

#endif // FLOW_H
