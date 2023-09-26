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
    for (int i; i != 1001; i++)
    {
        results.push_back(std::async(std::launch::async, do_concurrent_stuff, i, std::ref(sum)));
    }

    std::cout << sum << std::endl;
    return 0;
}

