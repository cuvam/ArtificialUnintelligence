#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Threshold for switching to insertion sort
#define INSERTION_SORT_THRESHOLD 16

// Forward declarations
void insertion_sort_range(int arr[], int low, int high);
void heap_sort_range(int arr[], int low, int high);
void sift_down(int arr[], int n, int i);

/**
 * Insertion sort for a range [low, high]
 * Adapted from insertion_sort.c
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
 * Adapted from heap_sort.c
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
 * Adapted from heap_sort.c to work on subarrays
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
 * Implements introspective sort - a hybrid sorting algorithm
 * that provides both fast average performance and optimal worst-case performance
 *
 * Algorithm:
 * 1. Starts with quicksort for good average-case performance
 * 2. Switches to heapsort if recursion depth exceeds 2*log(n) to avoid O(n²)
 * 3. Uses insertion sort for small subarrays (< 16 elements)
 *
 * Time complexity: O(n log n) worst case
 * Space complexity: O(log n) average case
 */
void introsort(int arr[], int n) {
    if (n <= 1) return;

    // Calculate maximum recursion depth: 2 * log2(n)
    int depth_limit = 2 * (int)(log(n) / log(2));

    introsort_util(arr, 0, n - 1, depth_limit);
}

/**
 * Utility function to print an array
 */
void print_array(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

/**
 * Main function to demonstrate introsort
 */
int main() {
    // Test case 1: Random array
    int arr1[] = {64, 34, 25, 12, 22, 11, 90, 88, 45, 50, 23, 9, 18, 77, 55};
    int n1 = sizeof(arr1) / sizeof(arr1[0]);

    printf("Test 1 - Random array:\n");
    printf("Original: ");
    print_array(arr1, n1);
    introsort(arr1, n1);
    printf("Sorted:   ");
    print_array(arr1, n1);
    printf("\n");

    // Test case 2: Already sorted array
    int arr2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int n2 = sizeof(arr2) / sizeof(arr2[0]);

    printf("Test 2 - Already sorted:\n");
    printf("Original: ");
    print_array(arr2, n2);
    introsort(arr2, n2);
    printf("Sorted:   ");
    print_array(arr2, n2);
    printf("\n");

    // Test case 3: Reverse sorted array
    int arr3[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    int n3 = sizeof(arr3) / sizeof(arr3[0]);

    printf("Test 3 - Reverse sorted:\n");
    printf("Original: ");
    print_array(arr3, n3);
    introsort(arr3, n3);
    printf("Sorted:   ");
    print_array(arr3, n3);
    printf("\n");

    // Test case 4: Small array (tests insertion sort path)
    int arr4[] = {5, 2, 8, 1, 9};
    int n4 = sizeof(arr4) / sizeof(arr4[0]);

    printf("Test 4 - Small array:\n");
    printf("Original: ");
    print_array(arr4, n4);
    introsort(arr4, n4);
    printf("Sorted:   ");
    print_array(arr4, n4);

    return 0;
}
