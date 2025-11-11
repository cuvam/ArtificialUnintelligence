#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Threshold for switching to insertion sort
#define INSERTION_SORT_THRESHOLD 16

// Color codes for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

// Forward declarations
void insertion_sort_range(int arr[], int low, int high);
void heap_sort_range(int arr[], int low, int high);
void sift_down(int arr[], int n, int i);
void swap(int *a, int *b);
int partition(int arr[], int low, int high);
void introsort_util(int arr[], int low, int high, int depth_limit);
void introsort(int arr[], int n);

/**
 * Insertion sort for a range [low, high]
 */
void insertion_sort_range(int arr[], int low, int high) {
    for (int i = low + 1; i <= high; i++) {
        int key = arr[i];
        int j = i - 1;

        while (j >= low && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

/**
 * Sift down operation to maintain max heap property
 */
void sift_down(int arr[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && arr[left] > arr[largest]) {
        largest = left;
    }

    if (right < n && arr[right] > arr[largest]) {
        largest = right;
    }

    if (largest != i) {
        int temp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = temp;
        sift_down(arr, n, largest);
    }
}

/**
 * Heap sort for a range [low, high]
 */
void heap_sort_range(int arr[], int low, int high) {
    int n = high - low + 1;
    int *subarray = &arr[low];

    // Build max heap
    for (int i = n / 2 - 1; i >= 0; i--) {
        sift_down(subarray, n, i);
    }

    // Extract elements from heap
    for (int i = n - 1; i > 0; i--) {
        int temp = subarray[0];
        subarray[0] = subarray[i];
        subarray[i] = temp;
        sift_down(subarray, i, 0);
    }
}

/**
 * Swap two elements
 */
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * Partition function for quicksort using median-of-three pivot selection
 */
int partition(int arr[], int low, int high) {
    // Median-of-three pivot selection
    int mid = low + (high - low) / 2;

    // Order low, mid, high
    if (arr[mid] < arr[low]) swap(&arr[low], &arr[mid]);
    if (arr[high] < arr[low]) swap(&arr[low], &arr[high]);
    if (arr[mid] < arr[high]) swap(&arr[mid], &arr[high]);

    // Place pivot at high-1
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

/**
 * Utility function for introsort with depth limit
 */
void introsort_util(int arr[], int low, int high, int depth_limit) {
    int size = high - low + 1;

    // Use insertion sort for small arrays
    if (size < INSERTION_SORT_THRESHOLD) {
        insertion_sort_range(arr, low, high);
        return;
    }

    // Switch to heapsort if recursion depth is too deep
    if (depth_limit == 0) {
        heap_sort_range(arr, low, high);
        return;
    }

    // Otherwise, use quicksort
    int pivot = partition(arr, low, high);
    introsort_util(arr, low, pivot - 1, depth_limit - 1);
    introsort_util(arr, pivot + 1, high, depth_limit - 1);
}

/**
 * Main introsort function
 */
void introsort(int arr[], int n) {
    if (n <= 1) return;

    // Calculate maximum recursion depth: 2 * log2(n)
    int depth_limit = 2 * (int)(log(n) / log(2));

    introsort_util(arr, 0, n - 1, depth_limit);
}

// ============================================================================
// TEST UTILITIES
// ============================================================================

/**
 * Check if an array is sorted in ascending order
 */
int is_sorted(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

/**
 * Check if two arrays contain the same elements (multiset equality)
 */
int arrays_equal_multiset(int arr1[], int arr2[], int n) {
    int *temp1 = (int *)malloc(n * sizeof(int));
    int *temp2 = (int *)malloc(n * sizeof(int));

    memcpy(temp1, arr1, n * sizeof(int));
    memcpy(temp2, arr2, n * sizeof(int));

    // Sort both arrays using a simple comparison
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (temp1[i] > temp1[j]) {
                swap(&temp1[i], &temp1[j]);
            }
            if (temp2[i] > temp2[j]) {
                swap(&temp2[i], &temp2[j]);
            }
        }
    }

    // Compare sorted arrays
    int result = 1;
    for (int i = 0; i < n; i++) {
        if (temp1[i] != temp2[i]) {
            result = 0;
            break;
        }
    }

    free(temp1);
    free(temp2);
    return result;
}

/**
 * Print an array
 */
void print_array(int arr[], int n) {
    printf("[");
    for (int i = 0; i < n; i++) {
        printf("%d", arr[i]);
        if (i < n - 1) printf(", ");
    }
    printf("]");
}

/**
 * Run a test case
 */
int run_test(const char *test_name, int arr[], int n) {
    printf("Testing: %-40s ", test_name);
    fflush(stdout);

    // Make a copy of the original array
    int *original = (int *)malloc(n * sizeof(int));
    memcpy(original, arr, n * sizeof(int));

    // Sort the array
    introsort(arr, n);

    // Check if sorted
    int sorted = is_sorted(arr, n);

    // Check if contains same elements
    int same_elements = arrays_equal_multiset(arr, original, n);

    free(original);

    if (sorted && same_elements) {
        printf("%s[PASS]%s\n", COLOR_GREEN, COLOR_RESET);
        return 1;
    } else {
        printf("%s[FAIL]%s\n", COLOR_RED, COLOR_RESET);
        if (!sorted) {
            printf("  Error: Array is not sorted\n");
        }
        if (!same_elements) {
            printf("  Error: Array elements were modified incorrectly\n");
        }
        return 0;
    }
}

// ============================================================================
// TEST CASES
// ============================================================================

int main() {
    int passed = 0;
    int total = 0;

    printf("\n");
    printf("========================================\n");
    printf("  INTROSORT TEST SUITE\n");
    printf("========================================\n\n");

    // Test 1: Empty array
    {
        int arr[] = {};
        total++;
        passed += run_test("Empty array", arr, 0);
    }

    // Test 2: Single element
    {
        int arr[] = {42};
        total++;
        passed += run_test("Single element", arr, 1);
    }

    // Test 3: Two elements (sorted)
    {
        int arr[] = {1, 2};
        total++;
        passed += run_test("Two elements (sorted)", arr, 2);
    }

    // Test 4: Two elements (unsorted)
    {
        int arr[] = {2, 1};
        total++;
        passed += run_test("Two elements (unsorted)", arr, 2);
    }

    // Test 5: Already sorted
    {
        int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        total++;
        passed += run_test("Already sorted array", arr, 10);
    }

    // Test 6: Reverse sorted
    {
        int arr[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
        total++;
        passed += run_test("Reverse sorted array", arr, 10);
    }

    // Test 7: All same elements
    {
        int arr[] = {5, 5, 5, 5, 5, 5, 5, 5};
        total++;
        passed += run_test("All same elements", arr, 8);
    }

    // Test 8: Duplicates
    {
        int arr[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
        total++;
        passed += run_test("Array with duplicates", arr, 11);
    }

    // Test 9: Small random array (tests insertion sort path)
    {
        int arr[] = {5, 2, 8, 1, 9, 3, 7};
        total++;
        passed += run_test("Small random array (size < 16)", arr, 7);
    }

    // Test 10: Medium random array
    {
        int arr[] = {64, 34, 25, 12, 22, 11, 90, 88, 45, 50, 23, 9, 18, 77, 55};
        total++;
        passed += run_test("Medium random array", arr, 15);
    }

    // Test 11: Large array with random values
    {
        int n = 1000;
        int *arr = (int *)malloc(n * sizeof(int));
        srand(12345);
        for (int i = 0; i < n; i++) {
            arr[i] = rand() % 10000;
        }
        total++;
        passed += run_test("Large random array (1000 elements)", arr, n);
        free(arr);
    }

    // Test 12: Array with negative numbers
    {
        int arr[] = {-5, 3, -2, 8, -10, 0, 15, -3};
        total++;
        passed += run_test("Array with negative numbers", arr, 8);
    }

    // Test 13: Array with negative and positive
    {
        int arr[] = {-100, -50, -25, 0, 25, 50, 100};
        total++;
        passed += run_test("Sorted negative to positive", arr, 7);
    }

    // Test 14: Pathological case - many duplicates
    {
        int arr[] = {1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5};
        total++;
        passed += run_test("Many duplicates (grouped)", arr, 15);
    }

    // Test 15: Worst case for naive quicksort (triggers heapsort)
    {
        int n = 100;
        int *arr = (int *)malloc(n * sizeof(int));
        // Create a pattern that would be bad for basic quicksort
        for (int i = 0; i < n; i++) {
            arr[i] = i % 10;
        }
        total++;
        passed += run_test("Pathological quicksort case", arr, n);
        free(arr);
    }

    // Test 16: Very large array
    {
        int n = 10000;
        int *arr = (int *)malloc(n * sizeof(int));
        srand(54321);
        for (int i = 0; i < n; i++) {
            arr[i] = rand() % 100000;
        }
        total++;
        passed += run_test("Very large array (10000 elements)", arr, n);
        free(arr);
    }

    // Test 17: Array at threshold boundary (16 elements)
    {
        int arr[] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
        total++;
        passed += run_test("Threshold size array (16 elements)", arr, 16);
    }

    // Test 18: Array just over threshold (17 elements)
    {
        int arr[] = {17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
        total++;
        passed += run_test("Just over threshold (17 elements)", arr, 17);
    }

    // Test 19: Alternating high/low
    {
        int arr[] = {100, 1, 99, 2, 98, 3, 97, 4, 96, 5};
        total++;
        passed += run_test("Alternating high/low values", arr, 10);
    }

    // Test 20: Maximum integer values
    {
        int arr[] = {2147483647, -2147483648, 0, 1000000, -1000000};
        total++;
        passed += run_test("Extreme integer values", arr, 5);
    }

    // Print summary
    printf("\n");
    printf("========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("Tests passed: %s%d/%d%s\n",
           passed == total ? COLOR_GREEN : COLOR_YELLOW,
           passed, total, COLOR_RESET);

    if (passed == total) {
        printf("%s✓ All tests passed!%s\n", COLOR_GREEN, COLOR_RESET);
        printf("========================================\n\n");
        return 0;
    } else {
        printf("%s✗ Some tests failed%s\n", COLOR_RED, COLOR_RESET);
        printf("========================================\n\n");
        return 1;
    }
}
