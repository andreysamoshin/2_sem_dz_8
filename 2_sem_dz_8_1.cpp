#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <vector>
#include <future>
#include <atomic>
#include "Timer.hpp"

double calc_pi_seq(std::size_t num_of_samples)
{
    std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
    std::uniform_real_distribution urd(0.0, 1.0);

    std::size_t num_of_samples_inside = 0U;

    for (auto i = 0U; i < num_of_samples; ++i)
    {
        double x = urd(gen);
        double y = urd(gen);

        if(x * x + y * y < 1.0)
            ++num_of_samples_inside;
    }
    
    return 4.0 * num_of_samples_inside / num_of_samples;
}

struct process_samples
{
    void operator() (std::atomic < std::size_t > & num_of_samples_inside, 
        std::size_t num_of_samples, std::uniform_real_distribution <double> & rdist, std::size_t id)
    {
        std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count() + id);

        for (auto i = 0U; i < num_of_samples; ++i)
        {
            double x = rdist(gen);
            double y = rdist(gen);

            if(x * x + y * y < 1.0)
                ++num_of_samples_inside;
        }
    }
};

double calc_pi_par(std::size_t num_of_samples)
{
    std::uniform_real_distribution urd(0.0, 1.0); // shared resource

    std::atomic < std::size_t > num_of_samples_inside = 0U;

	const std::size_t hardware_threads = std::thread::hardware_concurrency();

	const std::size_t num_threads = hardware_threads != 0 ? hardware_threads : 2;

    const std::size_t samples_per_thread = num_of_samples / num_threads;

    std::vector < std::future < void > > futures (num_threads - 1);

	for (std::size_t i = 0U; i < (num_threads - 1U); ++i)
        futures[i] = std::async(std::launch::async, process_samples(), std::ref(num_of_samples_inside),
        samples_per_thread, std::ref(urd), i); // i is treated as id for async

    process_samples () (num_of_samples_inside,
            samples_per_thread, urd, num_threads - 1U);

    for (std::size_t i = 0U; i < (num_threads - 1); ++i)
		futures[i].get();

	return 4.0 * num_of_samples_inside / num_of_samples;
}

int main()
{
    std::size_t num_of_samples = 10'000'000U;
    mytime::timer t(mytime::seconds{});
    double time; // seconds

    time = t.time();
    double pi_seq = calc_pi_seq(num_of_samples);
    time = t.time() - time;
    std::cout << "SEQ" << ": " << time << " seconds" <<std::endl;
    std::cout  << "PI = " << pi_seq << std::endl;

    time = t.time();
    double pi_par = calc_pi_par(num_of_samples);
    time = t.time() - time;
    std::cout << "PAR" << ": " << time << " seconds" <<std::endl;
    std::cout  << "PI = " << pi_par << std::endl;

    return EXIT_SUCCESS;
}