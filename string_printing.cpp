#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

class Printer{

    string str;
    int count;
    int noThreads;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done;
    std::atomic<int> currentCharCount;
    std::atomic<int> currentIndex;
    std::vector<std::thread> threads_;
public:
    Printer(string strPrint, int charCount, int numThreads) : str(strPrint), count(charCount), noThreads(numThreads), currentIndex(0){}
    
    void startPrinting(){
        int perThreadCount = str.size() / noThreads;
        for(int i = 1; i <= noThreads; i++){
            int threadId = i; 
            threads_.emplace_back([this, threadId](){
                while(!done){
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this, threadId](){
                            int left = (threadId - 1) * count;
                            int right = threadId * count;
                            int modCount =  (currentCharCount % (count * noThreads));
                            return modCount >= left && modCount < right || done;
                        });

                        //std::cout <<   threadId <<" current Index :" << currentIndex << std::endl;
                        std::cout <<   threadId <<" :" << str[currentIndex] << std::endl;
                        currentCharCount++;
                        currentIndex++;

                        if(currentIndex == str.size()){
                            currentIndex = 0;
                        }
                    }
                    cv.notify_all();
                }
            });
        }
    }

    void stop(){

        done = true;
        cv.notify_all();
        for(auto&t : threads_){
            if(t.joinable())
                t.join();
        }
    }
};


int main(){
    Printer p1("ABCDEFGHIJ", 3, 4);

    p1.startPrinting();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    p1.stop();

}


