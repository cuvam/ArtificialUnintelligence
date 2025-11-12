#include <stdio.h>

/**
 * Sift down operation to maintain max heap property
 * @param arr: array to heapify
 * @param n: size of the heap
 * @param i: index of the node to sift down
 */
void sift_down(int arr[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    // Check if left child exists and is greater than root
    if (left < n && arr[left] > arr[largest]) {
        largest = left;
    }

    // Check if right child exists and is greater than largest so far
    if (right < n && arr[right] > arr[largest]) {
        largest = right;
    }

    // If largest is not root, swap and continue sifting down
    if (largest != i) {
        int temp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = temp;

        // Recursively sift down the affected subtree
        sift_down(arr, n, largest);
    }
}

/**
 * Heap sort implementation using max heap
 * @param arr: array to sort
 * @param n: size of the array
 */
void heap_sort(int arr[], int n) {
    // Build max heap by calling sift_down on all non-leaf nodes
    // Start from the last non-leaf node (n/2 - 1) and move backwards
    for (int i = n / 2 - 1; i >= 0; i--) {
        sift_down(arr, n, i);
    }

    // Extract elements from heap one by one
    for (int i = n - 1; i > 0; i--) {
        // Move current root (maximum) to end
        int temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;

        // Call sift_down on the reduced heap
        sift_down(arr, i, 0);
    }
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
 * Main function to demonstrate heap sort
 */
int main() {
    int arr[] = {12, 11, 13, 5, 6, 7};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("Original array: ");
    print_array(arr, n);

    heap_sort(arr, n);

    printf("Sorted array: ");
    print_array(arr, n);

    return 0;
}
