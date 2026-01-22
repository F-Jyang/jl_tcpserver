#include <buffer.h>
#ifdef _WIN32
#include <locale.h>
#endif
#include <iostream>
#include <numeric>


int main(int argc, char const *argv[])
{

#ifdef _WIN32
    setlocale(LC_ALL, ".utf-8");
#endif

    //jl::Buffer buffer(2048);
    //buffer.Append(123);
    //buffer.Append(123.0f);
    //buffer.Append(123.0);
    //buffer.Append("hello world");
    //buffer.Append(std::vector<char>{'a', 'b', 'c'});
    //buffer.Append(std::vector<char>(2048, 20));
    //buffer.Append(std::vector<char>(2048, 20));
    //std::vector<int> result1 = buffer.Read<int>(1);
    //std::vector<float> result2 = buffer.Read<float>(1);
    //std::vector<double> result_float = buffer.Read<double>(1);
    //std::string result3 = buffer.ReadAsString(12); // "hello world" 类型是 char (&data)[12]
    //std::string result4 = buffer.ReadAsString(3);
    //std::vector<char> result5 = buffer.Read<char>(4096);
    //std::cout << buffer.Size() << std::endl;
    //std::cout << result1[0] << std::endl;
    //std::cout << result2[0] << std::endl;
    //std::cout << result_float[0] << std::endl;
    //std::cout << result3 << std::endl;
    //std::cout << result4 << std::endl;
    //int sum = std::accumulate(result5.begin(), result5.end(), 0);
    //assert(sum == 4096 * 20);
    //std::cout << buffer.Size() << std::endl;

    return 0;
}
