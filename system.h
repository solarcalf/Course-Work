#ifndef __SYSTEM_H__
#define __SYSTEM_H__


#include <functional>
#include <array>
#include <vector>
#include <random>
#include "RngStream.h"

double get_expon() {return 3.14;}

class System 
{
public:
    struct State;
    struct Timers;
    struct Statistics;
    

    using FP = double;
    // using State = std::array<FP, 3>;
    using Vector_of_stats = std::vector<Statistics>;
    using Conductor = std::function<std::array<FP, 3>(std::array<FP, 3>)>;


    struct State {
        std::array<size_t, 3> state;
        void refresh(size_t ind) {
            if (ind == 0)
            {
                // if (state[0] == male_queue_limit)
            }
        }
    };

    struct Statistics 
    {
        FP time_of_prostoy;
        std::vector<State> passed_states;
    };

    struct Timers
    {
        std::array<FP, 3> clocks;

        void set_clocks(); // Requires distribution

        std::pair<FP, size_t> get_min() 
        {
            if (clocks[0] < clocks[1] && clocks[0] < clocks[2]) 
                return {clocks[0], 0};
            else  if (clocks[1] < clocks[2]) 
                return {clocks[1], 1};
                
            return {clocks[2], 2};
        }

        void refresh(FP passed_time, size_t zero_clock_num) {
            for (size_t i = 0; i < 0; ++i) 
            {
                if (i != zero_clock_num) clocks[i] -= passed_time;
                else clocks[i] = get_expon();
            }
        }
    };


    Statistics run(const RngStream& rng) 
    {
        FP elapsed_time = 0;
        State state;
        Timers timers;
        Statistics obtained_stat;

        timers.set_clocks();

        while (elapsed_time < T) 
        {
            auto [passed_time, zero_clock_num] = timers.get_min();

            timers.refresh(passed_time, zero_clock_num);
            state.refresh(zero_clock_num);

            elapsed_time += passed_time;
        }

        return Statistics();
    }

    Vector_of_stats run(size_t n) 
    {
        Vector_of_stats stat_vector(n);

        #pragma parallel for
        for (size_t i = 0; i < n; ++i) 
        {
            RngStream rng;
            rng.AdvanceState(0, i * 1'000'000);
            stat_vector[i] = run(rng);
        }

        return stat_vector;
    }


    System(FP time = 100, FP l1 = 1, FP l2 = 1, FP mu = 1): T(time), l1(l1), l2(l2), mu(mu) {}


private:
    FP T;
    FP l1, l2, mu;
    size_t male_queue_limit = UINT32_MAX;
    size_t female_queue_limit = UINT32_MAX;
    Conductor conduct;
};


#endif // __SYSTEM_H__
