#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <mutex>
#include <thread>

std::string generate_sequence(const std::vector < char > & v, std::size_t n)
{
    std::mt19937 gen (std::chrono::system_clock().now().time_since_epoch().count());
    std::uniform_int_distribution uid (0, 3);

    std::string string(n, '0');

    for (auto & letter : string)
    {
        letter = v[uid(gen)];
    }

    return string;
}

class Threads_Guard
{
public:

	explicit Threads_Guard(std::vector < std::thread > & threads) :
		m_threads(threads)
	{}

	Threads_Guard(Threads_Guard const &) = delete;

	Threads_Guard& operator=(Threads_Guard const &) = delete;

	~Threads_Guard() noexcept
	{
		try
		{
			for (std::size_t i = 0; i < m_threads.size(); ++i)
			{
				if (m_threads[i].joinable())
				{
					m_threads[i].join();
				}
			}
		}
		catch (...)
		{
			// std::abort();
		}
	}

private:

	std::vector < std::thread > & m_threads;
};

template < typename Iterator, typename T, typename Container>
struct Searcher
{
	void operator()(Iterator first, Iterator block_start, Iterator block_end, T substring,
        Container & container, std::mutex & mutex) noexcept
	{
		try
		{
            auto substr_length = substring.size();
            for (auto find_it = block_start; find_it != block_end; ++find_it) // find_it - iterator that marks the beginning of a substring occurrence
            {
                bool found = true;
                for (auto str_it = find_it, substr_it = std::begin(substring); 
                str_it != std::next(find_it, substr_length); ++str_it, ++substr_it)
                {
                    if (*str_it != *substr_it)
                    {
                        found = false;
                        break;
                    }
                }
                if (found)
                {
                    std::scoped_lock <std::mutex> lock (mutex);
                    container.push_back(std::distance(first, find_it));
                }
            }
		}
		catch (...)
		{
		}
	}
};

template < typename Iterator, typename T, typename Container>
void parallel_find(Iterator first, Iterator last, T substring, Container & container)
{
    std::mutex mutex;
	
    int check_length = std::distance(first, last) + 1 - std::distance(std::end(substring), std::end(substring));

	if (check_length <= 0)
		return;

    const std::size_t find_range = static_cast < std::size_t > (check_length);

	const std::size_t min_per_thread = 1;
	const std::size_t max_threads =
		(find_range + min_per_thread - 1) / min_per_thread;

	const std::size_t hardware_threads =
		std::thread::hardware_concurrency();

	const std::size_t num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

	const std::size_t block_size = find_range / num_threads;

	std::vector < std::thread > threads(num_threads - 1);

	{
		Threads_Guard guard(threads);

		Iterator block_start = first;

		for (std::size_t i = 0; i < (num_threads - 1); ++i)
		{
			Iterator block_end = block_start;
			std::advance(block_end, block_size);

			threads[i] = std::thread(Searcher < Iterator, T, Container> (),
				first, block_start, block_end, substring, std::ref(container), std::ref(mutex));

			block_start = block_end;
		}

		Searcher < Iterator, T, Container> ()(first, block_start, last, substring, container, mutex);
	}
}

int main()
{
    std::vector < char > v = {'A', 'G', 'T', 'C'};
    std::string string = generate_sequence (v, 100);
    std::string string_to_find;

    std::cout << string << std::endl;
    std::cin  >> string_to_find;

    std::vector < std::size_t > indices;
    parallel_find(std::begin(string), std::end(string), string_to_find, indices);

    if (!indices.empty())
    {
        std::sort(std::begin(indices), std::end(indices), std::less<>());
        std::copy(std::begin(indices), std::end(indices), std::ostream_iterator < int > (std::cout, " "));
        std::cout << std::endl;
    }
    else
    {
        std::cout << "not found" << std::endl;
    }

    return EXIT_SUCCESS;
}