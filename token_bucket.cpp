// #include <iostream>
// #include <queue>
// #include <thread>
// #include <mutex>
// #include <condition_variable>

// class TokenBucket{
// public:
//     TokenBucket(int bucket_size) : max_bucket_(bucket_size){};

//     void addToken(int token){
//         {
//             std::unique_lock<std::mutex> lock(m);
//             full_cv.wait(lock, [this]{
//                 return tokens.size() < max_bucket_;
//             });
//             tokens.push(token);
//         }
//         empty_cv.notify_one();
//     }

//     int getToken(){
//         int token;
//         {
//             std::unique_lock<std::mutex> lock(m);
//             empty_cv.wait(lock, [this]{
//                 return tokens.size();
//             });
//             token = tokens.front();
//             tokens.pop();
//         }
//         full_cv.notify_one();

//         return token;
//     }
// private:
//     int max_bucket_;
//     std::mutex m;
//     std::queue<int> tokens;
//     std::condition_variable empty_cv;
//     std::condition_variable full_cv;
// };

// void addBucketToken(TokenBucket& bucket){
//     for(int i = 0; i < 100; i++){
//         bucket.addToken(i);
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//     }
    
// }

// int main(){

//     TokenBucket bucket(100);
//     std::thread addToken(addBucketToken, std::ref(bucket));
    
//     for(int i = 0; i < 50; i++){
//         std::cout << "Token Fetched : "<< bucket.getToken() << std::endl;
//     }

//     if(addToken.joinable())
//         addToken.join();

//     return 0;
// }


#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class BlockingTokenBucket {
public:
    BlockingTokenBucket(size_t capacity, double tokens_per_second)
        : capacity_(capacity),
          tokens_(capacity),
          refill_rate_(tokens_per_second),
          stop_(false) {
        refill_thread_ = std::thread(&BlockingTokenBucket::refillTokens, this);
    }

    ~BlockingTokenBucket() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        if (refill_thread_.joinable())
            refill_thread_.join();
    }

    // Blocks until at least 1 token is available, then consumes it
    void consume(size_t tokens = 1) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return tokens_ >= tokens || stop_; });

        if (stop_) return;

        tokens_ -= tokens;
    }

private:
    void refillTokens() {
        using namespace std::chrono;
        auto last = steady_clock::now();

        while (true) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (stop_) break;
            }

            auto now = steady_clock::now();
            double elapsed = duration_cast<microseconds>(now - last).count() / 1e6;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                tokens_ = std::min(capacity_, tokens_ + elapsed * refill_rate_);
                cv_.notify_all();
            }

            last = now;
            std::this_thread::sleep_for(milliseconds(100));
        }
    }

    const size_t capacity_;
    double tokens_;            // fractional tokens allowed
    const double refill_rate_; // tokens added per second
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_;
    std::thread refill_thread_;
};

int main() {
    BlockingTokenBucket limiter(10, 5); // 10 burst capacity, refills at 5 tokens/sec

    for (int i = 0; i < 20; ++i) {
        limiter.consume(); // Will block if necessary
        std::cout << "Request " << i << ": allowed\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // simulate request pacing
    }

    return 0;
}
