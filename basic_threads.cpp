#include <iostream>
#include <thread>
#include <atomic>

//std::atomic<int> counter(0);
int counter = 0;
std::mutex m;
void f1(){
    std::cout << "Thread Id :" << std::this_thread::get_id() << std::endl;
    for(int i = 0; i < 10000; i++){
        std::lock_guard<std::mutex> lock(m);
        counter++;
    }
    std::cout << " F1 function" << std::endl;
}

int main() { 

    std::vector<std::thread> threads;
    for(int i = 0; i < 10; i++){
        threads.emplace_back(f1);
    }

    for(int i = 0; i < threads.size(); i++){
        if(threads[i].joinable())
            threads[i].join();
    }

    std::cout << "Main Thread Id" << std::this_thread::get_id() << std::endl;

    //std::cout << "Counter Value" << counter.load() << std::endl;
    std::cout << "Counter Value" << counter << std::endl;

    int x;
    std::cin >> x;
 
    return 0;
}