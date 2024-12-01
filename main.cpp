#include <iostream>
#include <random>
#include <array>
#include <functional>
#include <chrono>
#include <cassert>
#include <climits>
#include "qsort.h"

const size_t MAX_N = 1e8;
std::mt19937_64 rnd(std::random_device{}());
std::array<unsigned long long, MAX_N> arr, test_arr;

typedef std::chrono::high_resolution_clock chrono_time;
using float_sec = std::chrono::duration<double>;
using float_time_point = std::chrono::time_point<chrono_time, float_sec>;

float_time_point get_cur_time() {
    return chrono_time::now();
}

void gen_test(unsigned long long mod, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        arr[i] = rnd() % mod;
    }
}

void copy_test(size_t len) {
    for (size_t i = 0; i < len; ++i) {
        test_arr[i] = arr[i];
    }
}

template<QsortTypes SortType>
double single_test(size_t len) {
    copy_test(len);

    auto startTime = get_cur_time();
    qsort<SortType>(test_arr.begin(), test_arr.begin() + len);
    double res = (get_cur_time() - startTime).count();

    for (size_t i = 0; i < len - 1; ++i) {
        assert(test_arr[i] <= test_arr[i + 1]);
    }

    return res;
}

int main() {
    printf("Number of workers = %s\n", getenv("PARLAY_NUM_THREADS"));

    const unsigned long long mods[] = {/*10, 1000,*/ ULLONG_MAX};
    const std::pair<size_t, unsigned int> test_groups[] = {
//            {1000, 100},
//            {100000, 50},
//            {1000000, 20},
//            {10000000, 10},
            {100000000, 5}
    };

    for (const auto [len, q]: test_groups) {
        for (const unsigned long long mod: mods) {
            double res[4];
            for (int i = 0; i < q; ++i) {
                gen_test(mod, len);
                res[static_cast<int>(QsortTypes::STD)] += single_test<QsortTypes::STD>(len);
                res[static_cast<int>(QsortTypes::SEQ)] += single_test<QsortTypes::SEQ>(len);
                res[static_cast<int>(QsortTypes::PAR_LINE)] += single_test<QsortTypes::PAR_LINE>(len);
                res[static_cast<int>(QsortTypes::PAR_LOG)] += single_test<QsortTypes::PAR_LOG>(len);
            }

            double seq_res = fmin(res[static_cast<int>(QsortTypes::STD)], res[static_cast<int>(QsortTypes::STD)]);
            printf("Len = %zu Mod = %llu\n", len, mod);
            printf("\tStd: %f s\n", res[static_cast<int>(QsortTypes::STD)] / q);
            printf("\tSeq: %f s\n", res[static_cast<int>(QsortTypes::SEQ)] / q);
            printf("\tPar (line span): %f s\n", res[static_cast<int>(QsortTypes::PAR_LINE)] / q);
            printf("\tPar (log span): %f s\n", res[static_cast<int>(QsortTypes::PAR_LOG)] / q);
            printf("\tSpeed-up (line span): x%f\n", seq_res / res[static_cast<int>(QsortTypes::PAR_LINE)]);
            printf("\tSpeed-up (log span): x%f\n", seq_res / res[static_cast<int>(QsortTypes::PAR_LOG)]);
        }
    }

    return 0;
}
