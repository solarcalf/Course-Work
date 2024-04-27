#include <vector>
#include <iostream>
#include <omp.h>
#include "system.h"

void test_system() {
    System system(300, 1.4, 1.4, 1);
    system.set_queues_limits(10, 10);
    System::Statistics stat;
    RngStream rng;

    stat = system.run(rng);

    for (auto& state : stat.passed_states)
        std::cout << state[0] << ' ' << state[1] << ' ' << state[2] << '\n';

    system.print();
    stat.print();
}

int main() {
    test_system();
}
