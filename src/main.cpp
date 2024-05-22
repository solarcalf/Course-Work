#include <vector>
#include <iostream>
#include <omp.h>
#include "./../include/system.h"

template <typename Container>
void print(Container cont) 
{
    for (const auto& x : cont)
        std::cout << x << ' ';
    std::cout << '\n';
}

void test_system() 
{
    System system(10, 1.6, 1.4, 2);
    // system.set_queues_limits(10, 10);
    RngStream rng;
    System::Statistics stat = system.run(rng);

    print(stat.male_interarrival_times);

    system.print();
    stat.print();
}

void test_distribution() 
{
    size_t N = 100;
    std::vector<double> v(N);
    RngStream rng;
    for (int i = 0; i < N; i++)
        v[i] = generate_exponential(2, rng);
    print(v);
}

void test_arrivals_times() 
{
    using namespace std;

    System system(100, 2, 1, 2.5);
    auto stat = system.run();
    
    vector<double> male_arrs = stat.male_interarrival_times;
    print(male_arrs);   
}

void validate() 
{
    System system(100, 3, 2, 1);
    validate_simulation(system, 100);
}

template <typename F>
std::vector<FP> operator+(const std::vector<F>& v, const std::vector<F>& w)
{
    std::vector<FP> ans(v.size());
    for (size_t i = 0; i < v.size(); ++i)
        ans[i] = static_cast<FP>(v[i]) + static_cast<FP>(w[i]);
    return ans;
}

int main() 
{
    using namespace std;

    System system(1000000, 1, 1, 3.5);
    auto stat = system.run();

    auto Y = stat.cycle_cost_value;
    auto a = stat.cycle_male_arrivals + stat.cycle_female_arrivals;

    auto [l, r] = regenerative_estimation(Y, a, 0.5);
    cout << l << ' ' << r << '\n';
}
