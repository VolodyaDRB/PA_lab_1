#pragma once

#include <parlay/primitives.h>

static inline std::mt19937_64 rnd_seq(std::random_device{}());

template<typename RandomIt>
void qsort_seq(RandomIt first, RandomIt last) {
    if (first + 1 >= last) return;

    size_t n = last - first;
    auto pivot = *(first + rnd_seq() % n);
    auto left = first;
    auto right = last - 1;

    while (left <= right) {
        while (*left < pivot) ++left;
        while (*right > pivot) --right;
        if (left <= right) {
            std::iter_swap(left++, right--);
        }
    }

    qsort_seq(first, right + 1);
    qsort_seq(left, last);
}

template<size_t BLOCK_SIZE = 10000, typename RandomIt>
void qsort_par_line(RandomIt first, RandomIt last) {
    if (first + 1 >= last) return;

    size_t n = last - first;
    if (n < BLOCK_SIZE) {
        qsort_seq(first, last);
        return;
    }

    auto pivot = *(first + rnd_seq() % n);
    auto left = first;
    auto right = last - 1;

    while (left <= right) {
        while (*left < pivot) ++left;
        while (*right > pivot) --right;
        if (left <= right) {
            std::iter_swap(left++, right--);
        }
    }

    parlay::parallel_do(
            [&] { qsort_par_line(first, right + 1); },
            [&] { qsort_par_line(left, last); }
    );
}

template<size_t BLOCK_SIZE = 10000, typename RandomIt>
void qsort_par_log(RandomIt first, RandomIt last) {
    using ValueType = typename std::iterator_traits<RandomIt>::value_type;

    if (first + 1 >= last) return;

    size_t n = last - first;
    if (n < BLOCK_SIZE) {
        qsort_seq(first, last);
        return;
    }

    auto pivot = *(first + rnd_seq() % n);
    auto left_seq = parlay::filter(std::ranges::subrange(first, last), [&](const auto &v) { return v < pivot; });
    auto mid_seq = parlay::filter(std::ranges::subrange(first, last), [&](const auto &v) { return v == pivot; });
    auto right_seq = parlay::filter(std::ranges::subrange(first, last), [&](const auto &v) { return v > pivot; });
    size_t left_size = left_seq.size(), mid_size = mid_seq.size();
    parlay::parallel_for(0, n, [&](size_t i) {
        if (i < left_size) {
            *(first + i) = left_seq[i];
        } else if (i < left_size + mid_size) {
            *(first + i) = mid_seq[i - left_size];
        } else {
            *(first + i) = right_seq[i - left_size - mid_size];
        }
    });

    parlay::parallel_do(
            [&] { qsort_par_log(first, first + left_size); },
            [&] { qsort_par_log(first + left_size + mid_size, last); }
    );
}

enum class QsortTypes {
    STD,
    SEQ,
    PAR_LINE,
    PAR_LOG
};

template<QsortTypes SortType, typename RandomIt>
void qsort(RandomIt first, RandomIt last) {
    if constexpr (SortType == QsortTypes::STD) {
        std::sort(first, last);
    } else if constexpr (SortType == QsortTypes::SEQ) {
        qsort_seq(first, last);
    } else if constexpr (SortType == QsortTypes::PAR_LINE) {
        qsort_par_line(first, last);
    } else {
        qsort_par_log(first, last);
    }
}
