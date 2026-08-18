#pragma once

#include <vector>

namespace sort {

template <typename T, typename Compare>
void quickSort(std::vector<T>& arr, Compare comp);

}

#include "quicksort.inl"
