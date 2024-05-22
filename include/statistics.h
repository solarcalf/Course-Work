#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#include <vector>
#include <numeric>
#include <cmath>
#include <array>
#include "system.h"


using FP = double;


FP inverse_standard_normal(FP p);

FP mean(const std::vector<FP>& data) 
{
    FP sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

FP variance(const std::vector<FP>& data, FP mean_value) 
{
    FP sum = 0.0;
    for (FP value : data) {
        sum += (value - mean_value) * (value - mean_value);
    }
    return sum / (data.size() - 1);
}

std::pair<FP, FP> confidence_interval(const std::vector<FP>& data, FP confidence_level = 0.95) 
{
    size_t n = data.size();
    FP mean_value = mean(data);
    FP var = variance(data, mean_value);
    FP stddev = std::sqrt(var);

    // квантиль t-распределения Стьюдента
    FP t = 1.96; // для уровня доверия 95% и большого числа степеней свободы

    FP margin_of_error = t * (stddev / std::sqrt(n));
    return {mean_value - margin_of_error, mean_value + margin_of_error};
}

std::array<FP, 2> regenerative_estimation(const std::vector<FP>& costs_per_cycle, const std::vector<FP>& clients_per_cycle, FP confidence_level)
{
    size_t n = costs_per_cycle.size();

    FP sum_of_costs = 0;
    FP sum_of_costs_squares = 0;

    FP sum_of_num_clients = 0;
    FP sum_of_num_clients_squares = 0;
    
    FP sum_of_products = 0;

    for (size_t i = 0; i < n; ++i)
    {
         sum_of_costs += costs_per_cycle[i];
         sum_of_costs_squares += std::pow(costs_per_cycle[i], 2);

         sum_of_num_clients += clients_per_cycle[i];
         sum_of_num_clients_squares += std::pow(clients_per_cycle[i], 2);

        sum_of_products += costs_per_cycle[i] * clients_per_cycle[i];
    }

    FP costs_mean = sum_of_costs / n;
    FP clients_num_mean = sum_of_num_clients / n;

    FP r_value = costs_mean / clients_num_mean;

    FP S11 = (1 / (n - 1)) * sum_of_costs_squares - (1 / (n * (n - 1))) * (sum_of_costs * sum_of_costs);
    FP S22 = (1 / (n - 1)) * sum_of_num_clients_squares - (1 / (n * (n - 1))) * (sum_of_num_clients * sum_of_num_clients);
    FP S12 = (1 / (n - 1)) * sum_of_products - (1 / (n * (n - 1))) * (sum_of_costs * sum_of_num_clients);
    FP S = std::sqrt(S11 - 2 * r_value * S12 + (r_value * r_value) * S22);

    FP quantile = inverse_standard_normal(1 - confidence_level / 2);
    FP margin_of_error = (quantile * S) / std::sqrt(clients_num_mean);

    return {r_value - margin_of_error, r_value + margin_of_error};
}

FP inverse_standard_normal(FP p) {
    // Constants for the approximation
    const FP a1 = -3.969683028665376e+01;
    const FP a2 = 2.209460984245205e+02;
    const FP a3 = -2.759285104469687e+02;
    const FP a4 = 1.383577518672690e+02;
    const FP a5 = -3.066479806614716e+01;
    const FP a6 = 2.506628277459239e+00;

    const FP b1 = -5.447609879822406e+01;
    const FP b2 = 1.615858368580409e+02;
    const FP b3 = -1.556989798598866e+02;
    const FP b4 = 6.680131188771972e+01;
    const FP b5 = -1.328068155288572e+01;

    const FP c1 = -7.784894002430293e-03;
    const FP c2 = -3.223964580411365e-01;
    const FP c3 = -2.400758277161838e+00;
    const FP c4 = -2.549732539343734e+00;
    const FP c5 = 4.374664141464968e+00;
    const FP c6 = 2.938163982698783e+00;

    const FP d1 = 7.784695709041462e-03;
    const FP d2 = 3.224671290700398e-01;
    const FP d3 = 2.445134137142996e+00;
    const FP d4 = 3.754408661907416e+00;

    // Define break-points
    const FP p_low = 0.02425;
    const FP p_high = 1.0 - p_low;

    // Rational approximation for lower region
    if (p < p_low) {
        FP q = std::sqrt(-2.0 * std::log(p));
        return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
               ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }

    // Rational approximation for upper region
    if (p > p_high) {
        FP q = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
               ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }

    // Rational approximation for central region
    FP q = p - 0.5;
    FP r = q * q;
    return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q /
           (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
}

#endif // __STATISTICS_H__
