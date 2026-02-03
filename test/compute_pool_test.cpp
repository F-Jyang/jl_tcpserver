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

    //void Compute()
    //{
    //    printf("finish.\n");
    //}


private:
    std::atomic<unsigned long long> sum_;
};

int main(int argc, char const *argv[])
{
    auto func_ptr = std::make_shared<FuncStruct>();
    unsigned long long sum = 0;
    auto &compute_pool = jl::ComputeThreadPool::GetInstance();
    auto start = std::chrono::system_clock::now();
    std::atomic<int> value = 0;
    std::future<int> f;
    for (std::size_t i = 0; i < 200000; i++)
    {
        sum += i + i + 1;
         //std::function<void()> task = std::bind(&FuncStruct::Compute, func_ptr, i, i + 1);
        //std::future<void> f1 = compute_pool.Post([=]()
        //                  { func_ptr->Compute(i, i + 1); });
        //std::future<void> f2 = compute_pool.Post([=]
        //                  { func_ptr->Compute(); });    
        //std::future<void> f2 = compute_pool.Post(&FuncStruct::Compute, func_ptr, i, i + 1);
        f = compute_pool.Post([&value]() {value += 1; return value.load(); });
        printf("%d\n", f.get());
    }
    std::cout << value.load() << std::endl;
    // compute_pool.Stop();
    auto end = std::chrono::system_clock::now();
    auto duration = end - start;
    std::cout << "Spend: " << std::chrono::duration_cast<std::chrono::seconds>(duration).count() << "s" << std::endl;
    return 0;
}
