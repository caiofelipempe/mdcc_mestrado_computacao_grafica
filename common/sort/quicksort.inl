#pragma once

#include <utility>
#include <algorithm>

namespace sort {

template <typename T, typename Compare>
int partition(std::vector<T>& arr, int low, int high, Compare comp) {
    const T& pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (comp(arr[j], pivot)) {
            ++i;
            std::swap(arr[i], arr[j]);
        }
    }

    std::swap(arr[i + 1], arr[high]);
    return i + 1;
}

template <typename T, typename Compare>
void quickSort(std::vector<T>& arr, Compare comp) {
    if (arr.size() <= 1)
        return;

    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(0, static_cast<int>(arr.size()) - 1);

    while (!stack.empty()) {
        auto [low, high] = stack.back();
        stack.pop_back();

        if (low >= high)
            continue;

        int p = partition(arr, low, high, comp);

        stack.emplace_back(low, p - 1);
        stack.emplace_back(p + 1, high);
    }
}

} // namespace sort