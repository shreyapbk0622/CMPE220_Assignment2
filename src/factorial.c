#include <stdio.h>

/*
 * Recursive factorial function.
 * factorial(n) = n * factorial(n - 1) for n > 1
 * factorial(0) = 1
 */
int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;  // base case
    }
    return n * factorial(n - 1);  // recursive case
}

/*
 * Driver program (main) that calls factorial().
 */
int main(void) {
    int n;

    printf("Enter a non-negative integer: ");
    if (scanf("%d", &n) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    if (n < 0) {
        printf("Factorial is not defined for negative numbers.\n");
        return 1;
    }

    int ans = factorial(n);
    printf("factorial(%d) = %d\n", n, ans);

    return 0;
}
