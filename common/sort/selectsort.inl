#pragma once

#include <utility>

namespace sort {

template <typename T, typename Compare>
void selectSort(std::vector<T>& arr, Compare comp) {
    const int n = static_cast<int>(arr.size());

    for (int i = 0; i < n - 1; ++i) {
        int minIndex = i;

        for (int j = i + 1; j < n; ++j) {
            if (comp(arr[j], arr[minIndex])) {
                minIndex = j;
            }
        }

        if (minIndex != i) {
            std::swap(arr[i], arr[minIndex]);
        }
    }
}

} // namespace sort