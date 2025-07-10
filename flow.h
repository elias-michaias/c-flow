#ifndef FLOW_H
#define FLOW_H

#ifndef PIPE_PLACEHOLDER
#define PIPE_PLACEHOLDER _
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    void *data;
    size_t len;
    size_t elem_size;
} Iterator;

/**
 * @brief Create an iterator from a static array.
 * @param arr The static array to convert.
 * @return An Iterator structure representing the array.
 */
#define to_iter(arr) \
    ((Iterator){ .data = (arr), .len = sizeof(arr)/sizeof(*(arr)), .elem_size = sizeof(*(arr)) })

/**
 * @brief Map each element of an iterator to a new value.
 * @param iter The input iterator.
 * @param in_type The type of each input element.
 * @param in_var The variable name for each input element.
 * @param out_type The type of each output element.
 * @param out_expr The expression to compute the output value.
 * @return Iterator of mapped values.
 */
#define iter_map(iter, in_type, in_var, out_type, out_expr) \
    ({ \
        Iterator input = (iter); \
        out_type *output = malloc(input.len * sizeof(out_type)); \
        for (size_t index = 0; index < input.len; ++index) { \
            in_type in_var = ((in_type*)input.data)[index]; \
            output[index] = (out_expr); \
        } \
        (Iterator){ .data = output, .len = input.len, .elem_size = sizeof(out_type) }; \
    })

/**
 * @brief Filter elements of an iterator by a predicate.
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param var The variable name for each element.
 * @param predicate The predicate expression (returns true to keep).
 * @return Iterator of filtered values.
 */
#define iter_filter(iter, type, var, predicate) \
    ({ \
        Iterator input = (iter); \
        type *output = malloc(input.len * sizeof(type)); \
        size_t count = 0; \
        for (size_t index = 0; index < input.len; ++index) { \
            type var = ((type*)input.data)[index]; \
            if (predicate) output[count++] = var; \
        } \
        (Iterator){ .data = output, .len = count, .elem_size = sizeof(type) }; \
    })

/**
 * @brief Reduce (sum) all elements of an iterator.
 * @param iter The input iterator.
 * @param type The type of each element (must be numeric).
 * @return The sum of all elements.
 */
#define iter_sum(iter, type) \
    ({ \
        Iterator input = (iter); \
        type sum = 0; \
        for (size_t index = 0; index < input.len; ++index) sum += ((type*)input.data)[index]; \
        sum; \
    })

/**
 * @brief Apply an operation to each element of an iterator (side effects only).
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param var The variable name for each element.
 * @param op The operation to perform (e.g., printf).
 * @return The original iterator (unchanged).
 */
#define iter_for(iter, type, var, op) \
    ({ \
        Iterator input = (iter); \
        for (size_t index = 0; index < input.len; ++index) { \
            type var = ((type*)input.data)[index]; \
            op; \
        } \
        input; \
    })

/**
 * @brief Take the first n elements from an iterator.
 * @param iter The input iterator.
 * @param n The number of elements to take.
 * @return Iterator of the first n elements.
 */
#define iter_take(iter, n) \
    ({ \
        Iterator input = (iter); \
        size_t count = (n) < input.len ? (n) : input.len; \
        (Iterator){ .data = input.data, .len = count, .elem_size = input.elem_size }; \
    })

/**
 * @brief Drop the first n elements from an iterator.
 * @param iter The input iterator.
 * @param n The number of elements to drop.
 * @return Iterator of the remaining elements.
 */
#define iter_drop(iter, n) \
    ({ \
        Iterator input = (iter); \
        size_t count = (n) < input.len ? (n) : input.len; \
        (Iterator){ .data = (char*)input.data + count * input.elem_size, .len = input.len - count, .elem_size = input.elem_size }; \
    })

/**
 * @brief Return a new iterator with elements in reverse order.
 * @param iter The input iterator.
 * @return Iterator with elements in reverse order.
 */
#define iter_reverse(iter) \
    ({ \
        Iterator input = (iter); \
        void* output = malloc(input.len * input.elem_size); \
        for (size_t index = 0; index < input.len; ++index) \
            memcpy((char*)output + index * input.elem_size, \
                   (char*)input.data + (input.len - 1 - index) * input.elem_size, \
                   input.elem_size); \
        (Iterator){ .data = output, .len = input.len, .elem_size = input.elem_size }; \
    })

/**
 * @brief Internal pointer-based version of iter_pad (do not use directly).
 * @param iter The input iterator.
 * @return Iterator with unique elements (first occurrence kept).
 */
#define iter_unique(iter) \
    ({ \
        Iterator input = (iter); \
        void* output = malloc(input.len * input.elem_size); \
        size_t count = 0; \
        for (size_t i = 0; i < input.len; ++i) { \
            int found = 0; \
            for (size_t j = 0; j < count; ++j) \
                if (memcmp((char*)input.data + i * input.elem_size, (char*)output + j * input.elem_size, input.elem_size) == 0) { found = 1; break; } \
            if (!found) \
                memcpy((char*)output + count++ * input.elem_size, (char*)input.data + i * input.elem_size, input.elem_size); \
        } \
        (Iterator){ .data = output, .len = count, .elem_size = input.elem_size }; \
    })

/**
 * @brief Concatenate two iterators of the same type.
 * @param iter1 The first input iterator.
 * @param iter2 The second input iterator.
 * @return Iterator with all elements of iter1 followed by iter2.
 */
#define iter_concat(iter1, iter2) \
    ({ \
        Iterator a = (iter1), b = (iter2); \
        void* output = malloc((a.len + b.len) * a.elem_size); \
        memcpy(output, a.data, a.len * a.elem_size); \
        memcpy((char*)output + a.len * a.elem_size, b.data, b.len * b.elem_size); \
        (Iterator){ .data = output, .len = a.len + b.len, .elem_size = a.elem_size }; \
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

/**
 * @brief Pad an iterator to a new length with a given value.
 * @param iter The input iterator.
 * @param newlen The new length of the output iterator.
 * @param padval The value to use for padding.
 * @return Iterator padded to newlen with padval.
 */
#define iter_pad(iter, newlen, padval) \
    ({ \
        __auto_type pad_value = (padval); \
        _iter_pad_ptr((iter), (newlen), &pad_value); \
    })

/**
 * @brief Repeat the iterator's elements a given number of times.
 * @param iter The input iterator.
 * @param times The number of times to repeat.
 * @return Iterator with repeated elements.
 */
#define iter_repeat(iter, times) \
    ({ \
        Iterator input = (iter); \
        size_t repeat_count = (times); \
        void* output = malloc(input.len * repeat_count * input.elem_size); \
        for (size_t i = 0; i < repeat_count; ++i) \
            memcpy((char*)output + i * input.len * input.elem_size, input.data, input.len * input.elem_size); \
        (Iterator){ .data = output, .len = input.len * repeat_count, .elem_size = input.elem_size }; \
    })

/**
 * @brief Left fold (accumulate from left to right).
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param acc_type The type of the accumulator.
 * @param acc The accumulator variable.
 * @param in_var The variable name for each input element.
 * @param init The initial value of the accumulator.
 * @param expr The expression to update the accumulator.
 * @return The final value of the accumulator.
 */
#define iter_foldl(iter, type, acc_type, acc, in_var, init, expr) \
    ({ \
        Iterator input = (iter); \
        acc_type acc = (init); \
        for (size_t index = 0; index < input.len; ++index) { \
            type in_var = ((type*)input.data)[index]; \
            acc = (expr); \
        } \
        acc; \
    })

/**
 * @brief Right fold (accumulate from right to left).
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param acc_type The type of the accumulator.
 * @param acc The accumulator variable.
 * @param in_var The variable name for each input element.
 * @param init The initial value of the accumulator.
 * @param expr The expression to update the accumulator.
 * @return The final value of the accumulator.
 */
#define iter_foldr(iter, type, acc_type, acc, in_var, init, expr) \
    ({ \
        Iterator input = (iter); \
        acc_type acc = (init); \
        for (ptrdiff_t index = (ptrdiff_t)input.len - 1; index >= 0; --index) { \
            type in_var = ((type*)input.data)[index]; \
            acc = (expr); \
        } \
        acc; \
    })
    
/**
 * @brief Zip two iterators into an iterator of pairtype (fields .a and .b).
 * @param it1type The type of elements in the first iterator.
 * @param it1 The first input iterator.
 * @param it2type The type of elements in the second iterator.
 * @param it2 The second input iterator.
 * @param pairtype The type of the resulting pairs.
 * @return Iterator of paired elements.
 */
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

/**
 * @brief Flatten an iterator of iterators (all inner iterators must have the same element type).
 * @param iter The input iterator of iterators.
 * @param itertype The type of each inner iterator.
 * @param elemtype The type of each element in the inner iterators.
 * @return Iterator of all elements, flattened.
 */
#define iter_flatten(iter, itertype, elemtype) \
    ({ \
        Iterator input = (iter); \
        size_t total = 0; \
        for (size_t i = 0; i < input.len; ++i) { \
            itertype inner = ((itertype*)input.data)[i]; \
            total += inner.len; \
        } \
        elemtype* output = malloc(total * sizeof(elemtype)); \
        size_t pos = 0; \
        for (size_t i = 0; i < input.len; ++i) { \
            itertype inner = ((itertype*)input.data)[i]; \
            for (size_t j = 0; j < inner.len; ++j) \
                output[pos++] = ((elemtype*)inner.data)[j]; \
        } \
        (Iterator){ .data = output, .len = total, .elem_size = sizeof(elemtype) }; \
    })

/**
 * @brief Split an iterator into two by predicate (returns a struct with .yes and .no fields).
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param var The variable name for each element.
 * @param predicate The predicate expression (returns true for yes branch).
 * @return Struct containing .yes and .no iterators.
 */
typedef struct { Iterator yes, no; } IteratorPartitionResult;
#define iter_partition(iter, type, var, predicate) \
    ({ \
        Iterator input = (iter); \
        type* yes_output = malloc(input.len * sizeof(type)); \
        type* no_output = malloc(input.len * sizeof(type)); \
        size_t yes_count = 0, no_count = 0; \
        for (size_t index = 0; index < input.len; ++index) { \
            type var = ((type*)input.data)[index]; \
            if (predicate) yes_output[yes_count++] = var; \
            else no_output[no_count++] = var; \
        } \
        (IteratorPartitionResult){ \
            .yes = (Iterator){ .data = yes_output, .len = yes_count, .elem_size = sizeof(type) }, \
            .no = (Iterator){ .data = no_output, .len = no_count, .elem_size = sizeof(type) } \
        }; \
    })

/**
 * @brief Prefix sum (or any scan operation), simplified.
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param var The variable name for each element.
 * @param init The initial value.
 * @param expr The expression to update the value.
 * @return Iterator of scanned values.
 */
#define iter_scan(iter, type, var, init, expr) \
    ({ \
        Iterator input = (iter); \
        type acc = (init); \
        type* output = malloc(input.len * sizeof(type)); \
        for (size_t index = 0; index < input.len; ++index) { \
            type var = ((type*)input.data)[index]; \
            acc = (expr); \
            output[index] = acc; \
        } \
        (Iterator){ .data = output, .len = input.len, .elem_size = sizeof(type) }; \
    })

/**
 * @brief Check if any element matches predicate.
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param var The variable name for each element.
 * @param predicate The predicate expression (returns true for a match).
 * @return 1 if any element matches, 0 otherwise.
 */
#define iter_any(iter, type, var, predicate) \
    ({ \
        Iterator input = (iter); \
        int found = 0; \
        for (size_t index = 0; index < input.len; ++index) { \
            type var = ((type*)input.data)[index]; \
            if (predicate) { found = 1; break; } \
        } \
        found; \
    })

/**
 * @brief Check if all elements match predicate.
 * @param iter The input iterator.
 * @param type The type of each element.
 * @param var The variable name for each element.
 * @param predicate The predicate expression (returns true for a match).
 * @return 1 if all elements match, 0 otherwise.
 */
#define iter_all(iter, type, var, predicate) \
    ({ \
        Iterator input = (iter); \
        int all = 1; \
        for (size_t index = 0; index < input.len; ++index) { \
            type var = ((type*)input.data)[index]; \
            if (!(predicate)) { all = 0; break; } \
        } \
        all; \
    })

/**
 * @brief Create an iterator over a range [start, end) (step=1).
 * @param type The type of each element.
 * @param start The starting value (inclusive).
 * @param end The ending value (exclusive).
 * @return Iterator over the range.
 */
#define iter_range(type, start, end) \
    ({ \
        type _s = (start), _e = (end); \
        size_t count = (_e > _s) ? (_e - _s) : 0; \
        type* output = malloc(count * sizeof(type)); \
        for (size_t index = 0; index < count; ++index) output[index] = _s + (type)index; \
        (Iterator){ .data = output, .len = count, .elem_size = sizeof(type) }; \
    })

/**
 * @brief Return a subrange [start, end) of the iterator.
 * @param iter The input iterator.
 * @param start The starting index (inclusive).
 * @param end The ending index (exclusive).
 * @return Iterator over the specified subrange.
 */
#define iter_slice(iter, start, end) \
    ({ \
        Iterator input = (iter); \
        size_t s = (start) < input.len ? (start) : input.len; \
        size_t e = (end) < input.len ? (end) : input.len; \
        (Iterator){ .data = (char*)input.data + s * input.elem_size, .len = (e > s ? e - s : 0), .elem_size = input.elem_size }; \
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
#include <Block.h>
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
