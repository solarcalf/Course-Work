#include <vector>
#include <iostream>
#include <omp.h>
#include "system.h"

std::vector<std::vector<double>> get_n_vectors_of_random_numbers(size_t n) {
    size_t numbers_per_vector = 10;
    std::vector<std::vector<double>> random_numbers(n, std::vector<double>(numbers_per_vector));

    #pragma omp parallel for
    for (size_t i = 0; i < n; i++) {
        // Создание объекта RngStream для каждого потока
        RngStream rng;

        // Прыжок в начало подпотока, уникального для каждого потока
        rng.AdvanceState(0, i * 1000000); // Прыжок на миллион шагов для каждого потока

        // Генерация случайных чисел
        for (size_t j = 0; j < numbers_per_vector; j++) {
            random_numbers[i][j] = rng.RandU01();
        }
    }

    return random_numbers;
}

int test_rng_stream() {
    size_t n = 5;  // Например, сгенерировать 5 векторов
    auto result = get_n_vectors_of_random_numbers(n);

    // Вывод результатов
    for (size_t i = 0; i < n; i++) {
        std::cout << "Vector " << i + 1 << ": ";
        for (double num : result[i]) {
            std::cout << num << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}


int main() {
     
}
