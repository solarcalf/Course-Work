#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <iostream>

#include <functional>
#include <array>
#include <vector>
#include <random>
#include "RngStream.h"


using FP = double;


FP generate_exponential(FP lambda, RngStream& rng) 
{
    FP u = rng.RandU01();
    return -std::log(u) / lambda;
}

std::function<std::array<size_t, 3>(std::array<size_t, 3>)> default_conductor = [](std::array<size_t, 3> state)
{
    if (state[2] != 0)
        return state;

    if (state[0] >= state[1])
    {
        state[2] = std::min(state[0], size_t(3));
        state[0] -= std::min(state[0], size_t(3));
    }
    else
    {
        state[2] = std::min(state[1], size_t(3));
        state[1] -= std::min(state[1], size_t(3));
    }

    return state;
};


class System 
{
public:
    struct Statistics;
    
    enum Event
    {
        MALE,
        FEMALE,
        SERVED,
        MALE_LEFT,
        FEMALE_LEFT
    };

private:
    using Vector_of_stats = std::vector<Statistics>;
    using Conductor = std::function<std::array<size_t, 3>(std::array<size_t, 3>)>;

    FP T;
    FP l1, l2, mu;
    size_t male_queue_limit = UINT32_MAX;
    size_t female_queue_limit = UINT32_MAX;
    Conductor conductor;

    struct State 
    {
        struct Timers
        {
            std::array<FP, 3> clocks;

            // Returns {time to next event, index of event}
            std::pair<FP, Event> get_event(const System& system, std::array<size_t, 3> state)
            {
                if (clocks[SERVED] == 0) // No one is in the system
                {
                    if (clocks[MALE] < clocks[FEMALE]) 
                    {
                        if (state[MALE] < system.male_queue_limit)
                            return {clocks[MALE], MALE};  // Male has come
                        else 
                            return {clocks[MALE], MALE_LEFT};
                    }
                    else
                    {
                        if (state[FEMALE] < system.female_queue_limit)
                            return {clocks[FEMALE], FEMALE};  // Female has come
                        else 
                            return {clocks[FEMALE], FEMALE_LEFT};
                    }
                }

                //  Someone in the system can be served
                if (clocks[MALE] < clocks[FEMALE] && clocks[MALE] < clocks[SERVED])
                { 
                    if (state[MALE] < system.male_queue_limit)
                        return {clocks[MALE], MALE};  // Male has come
                    else 
                        return {clocks[MALE], MALE_LEFT};
                }
                else  if (clocks[FEMALE] < clocks[SERVED]) 
                {
                    if (state[FEMALE] < system.female_queue_limit)
                        return {clocks[FEMALE], FEMALE};  // Female has come
                    else 
                        return {clocks[FEMALE], FEMALE_LEFT};
                }

                return {clocks[SERVED], SERVED};  // One person has been served
            }

            void refresh(FP passed_time, Event event, RngStream& rng, const System& system, const std::array<size_t, 3>& state) 
            {
                // Set time for serving if 
                // (First person has came in the system) OR (person has been served and there still someone in the system)
                if ((clocks[SERVED] == 0 || event == SERVED) && state[SERVED] != 0)
                    clocks[SERVED] = generate_exponential(system.mu, rng);

                // Refresh time for serving if we have to
                if (event != SERVED && clocks[SERVED] != 0)
                    clocks[SERVED] -= passed_time;

                // Last person was served
                if (event == SERVED && state[SERVED] == 0)
                    clocks[SERVED] = 0;

                auto [male_queue_limit, female_queue_limit] = system.get_queues_limits();

                // Male has come
                if (event == MALE) 
                {
                    clocks[MALE] = generate_exponential(system.l1, rng);
                    clocks[FEMALE] -= passed_time;
                }

                // Female has come
                if (event == FEMALE) 
                {
                    clocks[FEMALE] = generate_exponential(system.l2, rng);
                    clocks[MALE] -= passed_time;
                }

                // Male has left
                if (event == MALE_LEFT)
                    clocks[MALE] = generate_exponential(system.l1, rng);

                // Female has left
                if (event == FEMALE_LEFT)
                    clocks[FEMALE] = generate_exponential(system.l2, rng);
            }
        };

        std::array<size_t, 3> state;
        Timers timers;

        State(const System& system, RngStream& rng): state{0, 0, 0} 
        {
            timers.clocks[MALE] = generate_exponential(system.l1, rng);
            timers.clocks[FEMALE] = generate_exponential(system.l2, rng);
            timers.clocks[SERVED] = 0;
        }

        std::pair<FP, Event> move_to_next_state(const System& system, RngStream& rng) 
        {
            auto [passed_time, event] = timers.get_event(system, state);

            // Change current state according to event
            if (event == SERVED) --state[SERVED];
            else if (event == MALE || event == FEMALE) ++state[event];

            // Conduction
            state = system.conductor(state);

            timers.refresh(passed_time, event, rng, system, state);
            return {passed_time, event};
        }
    };

public:
    struct Statistics 
    {
        FP downtime;
        size_t total_male;
        size_t total_female;
        size_t male_left;
        size_t female_left;
        std::vector<FP> arrival_times;
        std::vector<std::array<size_t, 3>> passed_states{{0, 0, 0}};

        void print() const
        {
            std::cout << "\n========= STATISTICS =========\n";
            std::cout << "Total downtime:\t\t" << downtime << '\n';
            std::cout << "Total male:\t\t" << total_male << '\n';
            std::cout << "Total female:\t\t" << total_female << '\n';
            std::cout << "Male left:\t\t" << male_left << '\n';
            std::cout << "Female left:\t\t" << female_left << '\n';
            std::cout << "==============================\n";
        }
    };

    Statistics run(RngStream& rng) 
    {
        FP total_elapsed_time = 0;
        State state(*this, rng);
        Statistics obtained_stat;

        bool queue_was_empty = true;

        while (total_elapsed_time < T) 
        {
            auto [passed_time, event] = state.move_to_next_state(*this, rng);
            total_elapsed_time += passed_time;
            
            // Obtain statistics
            obtained_stat.passed_states.push_back(state.state);
            // obtained_stat.arrival_times.push_back(elapsed_time);

            if (!obtained_stat.arrival_times.empty())
                obtained_stat.arrival_times.push_back(total_elapsed_time + obtained_stat.arrival_times.back());
            else
                obtained_stat.arrival_times.push_back(total_elapsed_time);

            if (event == MALE) ++obtained_stat.total_male;
            else if (event == FEMALE) ++obtained_stat.total_female;
            else if (event == MALE_LEFT) ++obtained_stat.male_left;
            else if (event == FEMALE_LEFT) ++obtained_stat.female_left;

            if (queue_was_empty) obtained_stat.downtime += passed_time;    
            queue_was_empty = !(state.state[MALE] || state.state[FEMALE]);
        }

        return obtained_stat;
    }

    // Runs n different experiments using multi-threading
    Vector_of_stats run(size_t n) 
    {
        Vector_of_stats stat_vector(n);

        #pragma omp parallel for
        for (size_t i = 0; i < n; ++i) 
        {
            RngStream rng;
            rng.AdvanceState(0, i * 1'000'000);
            stat_vector[i] = run(rng);
        }

        return stat_vector;
    }


    System(FP time = 100, FP l1 = 1, FP l2 = 1, FP mu = 1, Conductor conductor = default_conductor): 
    T(time), l1(l1), l2(l2), mu(mu), conductor(conductor) {}

    void set_queues_limits(size_t male_queue_limit, size_t female_queue_limit)
    {
        this->male_queue_limit = male_queue_limit;
        this->female_queue_limit = female_queue_limit;
    }

    std::array<size_t, 2> get_queues_limits() const 
    {
        return {male_queue_limit, female_queue_limit};
    }

    void print() const
    {
        std::cout << "\n======= SYSTEM SUMMARY =======\n";
        std::cout << "Total time of work:\t" << T << '\n';
        std::cout << "Parament for male:\t" << l1 << '\n';
        std::cout << "Parament for female:\t" << l2 << '\n';
        std::cout << "Parament for serve:\t" << mu << '\n';
        std::cout << "Male queue limit:\t" << male_queue_limit << '\n';
        std::cout << "Female queue limit:\t" << female_queue_limit << '\n';
        std::cout << "==============================\n";
    }
};


#endif // __SYSTEM_H__
