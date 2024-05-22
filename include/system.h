#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <iostream>
#include <functional>
#include <array>
#include <vector>
#include "statistics.h"
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

std::function<FP(std::array<size_t, 3>, FP)> default_cost_function = [](std::array<size_t, 3> state, FP time)
{
    return (state[0] + state[1]) * time;
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
    std::function<FP(std::array<size_t, 3>, FP)> cost_function;
    Conductor conductor;

    struct State 
    {
        struct Timers
        {
            std::array<FP, 3> clocks;

            // Returns {time to next event, event}
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

            void refresh(FP passed_time, const Event event, RngStream& rng, const System& system, const std::array<size_t, 3>& state) 
            {
                // Set time for serving if 
                // (First person has came in the system) OR (person has been served and there still someone in the system)
                if (event == SERVED) {
                    clocks[MALE] -= passed_time;
                    clocks[FEMALE] -= passed_time;
                }
                if ((clocks[SERVED] == 0 || event == SERVED) && state[SERVED] != 0)
                    clocks[SERVED] = generate_exponential(system.mu, rng);
                // Refresh time for serving if we have to
                else if (event != SERVED && clocks[SERVED] != 0)
                    clocks[SERVED] -= passed_time;
                // Last person was served
                else if (event == SERVED && state[SERVED] == 0)
                    clocks[SERVED] = 0;

                auto [male_queue_limit, female_queue_limit] = system.get_queues_limits();

                // Male has come or left
                if (event == MALE || event == MALE_LEFT) 
                {
                    clocks[MALE] = generate_exponential(system.l1, rng);
                    clocks[FEMALE] -= passed_time;
                }

                // Female has come or left
                if (event == FEMALE || event == FEMALE_LEFT) 
                {
                    clocks[FEMALE] = generate_exponential(system.l2, rng);
                    clocks[MALE] -= passed_time;
                }
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
        FP downtime = 0;
        size_t total_male = 0;
        size_t total_female = 0;
        size_t total_male_left = 0;
        size_t total_female_left = 0;
        std::vector<std::array<size_t, 3>> passed_states{{0, 0, 0}};

        std::vector<FP> male_interarrival_times;
        std::vector<FP> female_interarrival_times;
        std::vector<FP> male_left_interarrival_times;
        std::vector<FP> female_left_interarrival_times;

        std::vector<FP> cycle_durations;
        std::vector<FP> cycle_cost_value;
        std::vector<size_t> cycle_male_arrivals;
        std::vector<size_t> cycle_female_arrivals;
        std::vector<size_t> cycle_male_left;
        std::vector<size_t> cycle_female_left;

        void print() const
        {
            std::cout << "\n========= STATISTICS =========\n";
            std::cout << "Total downtime:\t\t" << downtime << '\n';
            std::cout << "Total male:\t\t" << total_male << '\n';
            std::cout << "Total female:\t\t" << total_female << '\n';
            std::cout << "Male left:\t\t" << total_male_left << '\n';
            std::cout << "Female left:\t\t" << total_female_left << '\n';
            std::cout << "==============================\n";
        }
    };

    Statistics run(RngStream rng = RngStream()) 
    {
        FP total_elapsed_time = 0;
        State state(*this, rng);
        Statistics obtained_stat;
    
        FP cycle_start_time = 0;
        size_t cycle_male_count = 0;
        size_t cycle_female_count = 0;
        size_t cycle_male_left_count = 0;
        size_t cycle_female_left_count = 0;

        bool queue_was_empty = true;
        FP cycle_cost_value = 0;

        FP last_male_arrival_time = 0;
        FP last_female_arrival_time = 0;
        FP last_male_left_arrival_time = 0;
        FP last_female_left_arrival_time = 0;

        while (total_elapsed_time < T) 
        {
            State previous_state = state;
            auto [passed_time, event] = state.move_to_next_state(*this, rng);
            total_elapsed_time += passed_time;

            cycle_cost_value += cost_function(previous_state.state, passed_time);


            // We can reduce all these checks by storing all cycle data as vector of arrays or smth like that
            // Obtain data for current cycle
            if (event == MALE) 
            {
                ++cycle_male_count;
                obtained_stat.male_interarrival_times.push_back(total_elapsed_time - last_male_arrival_time);
                last_male_arrival_time = total_elapsed_time;
            }
            else if (event == FEMALE) 
            {
                ++cycle_female_count;
                obtained_stat.female_interarrival_times.push_back(total_elapsed_time - last_female_arrival_time);
                last_female_arrival_time = total_elapsed_time;
            }
            else if (event == MALE_LEFT) 
            {
                ++cycle_male_left_count;
                obtained_stat.male_left_interarrival_times.push_back(total_elapsed_time - last_male_left_arrival_time);
                last_male_left_arrival_time = total_elapsed_time;
            }
            else if (event == FEMALE_LEFT) 
            {
                ++cycle_female_left_count;
                obtained_stat.female_left_interarrival_times.push_back(total_elapsed_time - last_female_left_arrival_time);
                last_female_left_arrival_time = total_elapsed_time;
            }

            // Check for regenerative condition
            if (is_regenerative_state(state.state)) 
            {
                obtained_stat.cycle_durations.push_back(total_elapsed_time - cycle_start_time);
                obtained_stat.cycle_male_arrivals.push_back(cycle_male_count);
                obtained_stat.cycle_female_arrivals.push_back(cycle_female_count);
                obtained_stat.cycle_male_left.push_back(cycle_male_left_count);
                obtained_stat.cycle_female_left.push_back(cycle_female_left_count);
                obtained_stat.cycle_cost_value.push_back(cycle_cost_value);

                // Renew couners for new cycle
                cycle_start_time = total_elapsed_time;
                cycle_male_count = 0;
                cycle_female_count = 0;
                cycle_male_left_count = 0;
                cycle_female_left_count = 0;
                cycle_cost_value = 0;
            }

            // Obtain general data
            if (queue_was_empty) obtained_stat.downtime += passed_time;    
            queue_was_empty = !(state.state[MALE] || state.state[FEMALE]);
        }

        obtained_stat.total_male = obtained_stat.male_interarrival_times.size();
        obtained_stat.total_female = obtained_stat.female_interarrival_times.size();
        obtained_stat.total_male_left = obtained_stat.male_left_interarrival_times.size();
        obtained_stat.total_female_left = obtained_stat.female_left_interarrival_times.size();

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
            stat_vector[i] = run(rng);
        }

        return stat_vector;
    }


    System(FP time = 100, FP l1 = 1, FP l2 = 1, FP mu = 1, Conductor conductor = default_conductor, 
    std::function<FP(std::array<size_t, 3>, FP)> cost_function = default_cost_function): 
            T(time), l1(l1), l2(l2), mu(mu), conductor(conductor), cost_function(cost_function) {}

    void set_queues_limits(size_t male_queue_limit, size_t female_queue_limit)
    {
        this->male_queue_limit = male_queue_limit;
        this->female_queue_limit = female_queue_limit;
    }

    void set_conductor(Conductor new_conductor)
    {
        this->conductor = new_conductor;
    }

    void set_cost_function(std::function<FP(std::array<size_t, 3>, FP)> new_cost_function)
    {
        this->cost_function = new_cost_function;
    }

    std::array<size_t, 2> get_queues_limits() const 
    {
        return {male_queue_limit, female_queue_limit};
    }

    std::array<FP, 3> get_distribution_params() const 
    {
        return {l1, l2, mu};
    }

    void print() const
    {
        std::cout << "\n======= SYSTEM SUMMARY =======\n";
        std::cout << "Total time of work:\t" << T << '\n';
        std::cout << "Parament for male:\t" << l1 << '\n';
        std::cout << "Parament for female:\t" << l2 << '\n';
        std::cout << "Parament for serving:\t" << mu << '\n';
        std::cout << "Male queue limit:\t" << male_queue_limit << '\n';
        std::cout << "Female queue limit:\t" << female_queue_limit << '\n';
        std::cout << "==============================\n";
    }

private:
    bool is_regenerative_state(const std::array<size_t, 3>& state) 
    {
        return (state[0] + state[1] + state[2]) == 0;
    }

};


void validate_simulation(System& system, size_t num_experiments) 
{
    std::vector<FP> male_interarrival_times;
    for (size_t i = 0; i < num_experiments; ++i) {
        RngStream rng;
        System::Statistics stats = system.run(rng);
        male_interarrival_times.insert(male_interarrival_times.end(), 
                                       stats.male_interarrival_times.begin(), 
                                       stats.male_interarrival_times.end());
    }

    if (male_interarrival_times.empty()) {
        std::cout << "No valid male interarrival times were collected.\n";
        return;
    }

    FP l1 = system.get_distribution_params()[0];
    FP mean_interarrival_time = mean(male_interarrival_times);
    FP expected_mean = 1 / l1; // ожидаемое значение для экспоненциального распределения с параметром lambda = l1

    std::cout << "Mean interarrival time: " << mean_interarrival_time << "\n";
    std::cout << "Expected mean: " << expected_mean << "\n";
    std::cout << "Difference: " << std::abs(mean_interarrival_time - expected_mean) << "\n";

    auto [ci_lower, ci_upper] = confidence_interval(male_interarrival_times);
    std::cout << "95% Confidence Interval for mean interarrival time: [" << ci_lower << ", " << ci_upper << "]" << std::endl;
}

#endif // __SYSTEM_H__
