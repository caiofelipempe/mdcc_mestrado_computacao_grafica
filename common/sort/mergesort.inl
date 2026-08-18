#pragma once

#include <algorithm>

namespace sort {

template <typename T, typename Compare>
static void mergeBlocks(
    std::vector<T>& arr,
    std::vector<T>& buffer,
    int left,
    int mid,
    int right,
    Compare comp
) {
    int i = left;
    int j = mid;
    int k = left;

    while (i < mid && j < right) {
        if (comp(arr[i], arr[j]))
            buffer[k++] = arr[i++];
        else
            buffer[k++] = arr[j++];
    }

    while (i < mid)
        buffer[k++] = arr[i++];

    while (j < right)
        buffer[k++] = arr[j++];
}

template <typename T, typename Compare>
void mergeSort(std::vector<T>& arr, Compare comp) {
    const int n = static_cast<int>(arr.size());
    if (n <= 1)
        return;

    std::vector<T> buffer(n);

    for (int width = 1; width < n; width *= 2) {
        for (int left = 0; left < n; left += 2 * width) {
            int mid   = std::min(left + width, n);
            int right = std::min(left + 2 * width, n);

            mergeBlocks(arr, buffer, left, mid, right, comp);
        }

        for (int i = 0; i < n; ++i)
            arr[i] = buffer[i];
    }
}

} // namespace sort