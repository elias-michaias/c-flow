#include "./flow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int add(int a, int b) {
    return a + b;
}

int mul(int a, int b) {
    return a * b;
}

int sub(int a, int b) {
    return a - b;
}

double int_to_double(int x) { return x * 1.26; }
char* double_to_str(double d) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", d);
    return buf;
}
size_t str_len(char* s) { return strlen(s); }
int size_t_to_int(size_t n) { return (int)(n * 10); }

// Example: Clang blocks-based currying for arity 5
float add5(float a, float b, float c, float d, float e) {
    return a + b + c + d + e;
}

int main(int argc, char *argv[]) {

    // chain(...): chain(x, f, g, h) -> h(g(f(x)))
    // only supports unary functions, but functions can return different types
    int chain_result = chain(
        7, 
        int_to_double, 
        double_to_str, 
        str_len, 
        size_t_to_int
    );

    printf("Chain Test:\n%d\n---\n", chain_result);

    // pipe(...): pipe(x, f, g, h) -> _ = f(x); _ = g(_); _ = h(_); _;
    // supports functions of any arity, but all functions must return same type
    int temp = pipe(
        add(3, 2),
        sub(4, _),
        mul(5, _),
        sub(_, 1),
        add(_, _)
    );

    printf("Pipe Test:\n%d\n---\n", temp);

    int arr[] = {1, 2, 3, 4, 5, 6, 7};

    printf("Iterator Test 1:\n");

    // Create an iterator from an array
    // and apply a series of transformations using the pipe macro
    // iter_map(arr, input_type, input_var, output_type, output_expr)
    Iterator t1 = pipe(
        to_iter(arr),
        iter_map(_, int, x, int, x * 7),
        iter_for(_, double, x, printf("%.2f ", x)),
        iter_drop(_, 2),
        iter_for(_, double, x, printf("%.2f ", x))
    );

    int arr2[] = {1, 2, 2, 3, 4};
    printf("\n=====\nIterator Test 2:\n---\n");
    Iterator t2;

    // STEP 1: Original
    printf("STEP 1: Original\n");
    t2 = to_iter(arr2);
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 2: Repeat
    printf("STEP 2: Repeat\n");
    t2 = iter_repeat(t2, 2);
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 3: Concat
    printf("STEP 3: Concat\n");
    t2 = iter_concat(t2, to_iter(arr2));
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 4: Reverse
    printf("STEP 4: Reverse\n");
    t2 = iter_reverse(t2);
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 5: Unique
    printf("STEP 5: Unique\n");
    t2 = iter_unique(t2);
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 6: Pad
    printf("STEP 6: Pad\n");
    t2 = iter_pad(t2, 10, 99);
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 7: Slice
    printf("STEP 7: Slice\n");
    t2 = iter_slice(t2, 2, 7);
    iter_for(t2, int, x, printf("%d ", x));
    printf("\n---\n");

    // STEP 8: Sum
    printf("STEP 8: Sum\n");
    int sum = iter_sum(t2, int);
    printf("%d ", sum);
    printf("\n---\n");

    // Iterator Test 3: foldl, foldr, zip
    printf("\n=====\nIterator Test 3:\n---\n");
    int arr3[] = {10, 20, 30, 40};
    int arr4[] = {1, 2, 3, 4};
    Iterator it3 = to_iter(arr3);
    Iterator it4 = to_iter(arr4);

    // foldl: sum
    int sum_l = iter_foldl(it3, int, int, acc, x, 0, acc + x);
    printf("foldl sum: %d\n---\n", sum_l);

    // foldr: subtract right-to-left
    int sub_r = iter_foldr(it3, int, int, acc, x, 0, x - acc);
    printf("foldr subtract: %d\n---\n", sub_r);

    // zip: pairwise sum
    // zip output types must be a struct with fields .a and .b
    typedef struct { int a, b; } pair_int_int;
    typedef struct { int a; char* b; } pair_int_string;
    Iterator zipped = iter_zip(int, it3, int, it4, pair_int_int);
    printf("zip (a,b): ");
    for (size_t i = 0; i < zipped.len; ++i) {
        pair_int_int p = ((pair_int_int*)zipped.data)[i];
        printf("(%d,%d) ", p.a, p.b);
    }
    printf("\n---\n");

    // zip with map: sum pairs
    Iterator zipped2 = iter_zip(int, it3, int, it4, pair_int_int);
    Iterator zipped_sum = iter_map(zipped2, pair_int_int, p, int, p.a + p.b);
    printf("zip sum: ");
    iter_for(zipped_sum, int, x, printf("%d ", x));
    printf("\n---\n");

    // Iterator Test 4: Advanced Functional Iterators
    printf("\n=====\nIterator Test 4: Advanced Functional Iterators\n---\n");
    // iter_zip_with: elementwise multiply (use zip+map)
    Iterator zipped_mul = iter_map(iter_zip(int, it3, int, it4, pair_int_int), pair_int_int, p, int, p.a * p.b);
    printf("zip+map (a*b): ");
    iter_for(zipped_mul, int, x, printf("%d ", x));
    printf("\n---\n");

    // iter_flatten: flatten array of iterators
    Iterator arr_iters[2] = { to_iter(arr3), to_iter(arr4) };
    Iterator iters_iter = to_iter(arr_iters);
    Iterator flat = iter_flatten(iters_iter, Iterator, int);
    printf("flatten: ");
    iter_for(flat, int, x, printf("%d ", x));
    printf("\n---\n");

    // iter_partition: even/odd split
    IteratorPartitionResult part = iter_partition(it3, int, x, x % 3 == 0);
    printf("partition (x mod 3 == 0): ");
    iter_for(part.yes, int, x, printf("%d ", x));
    printf(" | (x mod 3 != 0): ");
    iter_for(part.no, int, x, printf("%d ", x));
    printf("\n---\n");

    // iter_scan: prefix sum
    Iterator scan = iter_scan(it3, int, x, 0, acc + x);
    printf("scan (prefix sum): ");
    iter_for(scan, int, x, printf("%d ", x));
    printf("\n---\n");

    // iter_any: any == 20
    int any20 = iter_any(it3, int, x, x == 20);
    printf("any == 20: %s\n---\n", any20 ? "yes" : "no");

    // iter_all: all > 0
    int allpos = iter_all(it3, int, x, x > 0);
    printf("all > 0: %s\n---\n", allpos ? "yes" : "no");

    // iter_range: 5..10
    Iterator rng = iter_range(int, 5, 10);
    printf("range 5..10: ");
    iter_for(rng, int, x, printf("%d ", x));
    printf("\n---\n");

    #ifdef __clang__
    // Partial application: manually curry add5 to get a function of 4 args
    __auto_type add5_curried = curry(add5, float, float, float, float, float);
    __auto_type add13 = add5_curried(10)(1)(2); // This line is for demonstration
    float result_partial = add13(4)(5);
    printf("Curried add5 (partial, a1=10, curry): %f\n", result_partial);
    #endif

    return 0;
}