#pragma once

#include <vector>

namespace sort {

template <typename T, typename Compare>
void mergeSort(std::vector<T>& arr, Compare comp);

} // namespace sort

#include "mergesort.inl"