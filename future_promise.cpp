#include <iostream>
#include <future>
#include <type_traits>

int calculateSum(int a, int b){
    int c = 0;
    for(int i = 0; i < 1000; i++){
        c += a + b;
    }
    return c;
}

int main(){

    using ReturnType = std::invoke_result_t<decltype(calculateSum), int, int>;

    // There are two behaviours std::launch::async and std::launch::deferred
    // if we dont defien anything it can be either of them
    // std::launch::deferred is kind of lazy starting where it delays till we call get or wait.

    std::future<ReturnType> fut = std::async(std::launch::async, calculateSum, 4, 5);
    std::cout << "Value : " << fut.get() << std::endl;

    // std::promise<int> prm;
    // std::future<int> fut = prm.get_future();

    // std::thread t([&prm]() {
    //     prm.set_value(calculateSum(4, 5));
    // });
    // std::cout << fut.get() << '\n';
    // t.join();

    return 0;

}