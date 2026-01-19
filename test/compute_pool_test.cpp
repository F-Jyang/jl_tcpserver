#include <compute_pool.hpp>
#include <iostream>
#include <assert.h>
#include <chrono>

struct FuncStruct
{
public:
    FuncStruct() : sum_(0) {}

    void Compute(int a, int b)
    {
        sum_ += a + b;
        printf("thread:%u, sum: %llu.\n", std::this_thread::get_id(), sum_.load());
    }

    void Compute()
    {
        printf("finish.\n");
    }


private:
    std::atomic<unsigned long long> sum_;
};

int main(int argc, char const *argv[])
{
    auto func_ptr = std::make_shared<FuncStruct>();
    unsigned long long sum = 0;
    auto &compute_pool = jl::ComputeThreadPool::GetInstance();
    auto start = std::chrono::system_clock::now();
    for (std::size_t i = 0; i < 100000; i++)
    {
        sum += i + i + 1;
        // std::function<void()> task = std::bind(&FuncStruct::Compute, &func, i, i + 1);
        compute_pool.Post([=]()
                          { func_ptr->Compute(i, i + 1); });
        compute_pool.Post([=]
                          { func_ptr->Compute(); });
        // compute_pool.Post(&FuncStruct::Compute, &func);
    }
    // compute_pool.Stop();
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;
    std::cout << "Spend: " << std::chrono::duration_cast<std::chrono::seconds>(duration).count() << "s" << std::endl;
    return 0;
}
