#include <future>
#include <vector>
#include <iostream>

static std::mutex trigger_guard;
std::vector<std::future<void>> results;

void do_concurrent_stuff(int i, int& sum)
{
    std::lock_guard<std::mutex> lock(trigger_guard);
    sum += i;
}

int main()
{

    int sum = 0;
    const int n_max_threads = 10;
    for (int i; i < 100; i++)
    {
        while (results.size() > n_max_threads)
        {
            std::cout << results.size() << std::endl;
            for (int thread = 0; thread == results.size(); thread++)
            {
                if (results[thread].wait_for(std::chrono::seconds(0)) == std::future_status::ready){results.erase(results.begin() + thread);}
            } 
        }
        results.push_back(std::async(std::launch::async, do_concurrent_stuff, i, std::ref(sum)));
    }

    std::cout << sum << std::endl;
    return 0;
}

