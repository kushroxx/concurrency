// #include <iostream>

// template <typename T>
// class BBQueue{

//     std::queue<T> bbqueue;
//     std::mutex mtx;
//     std::condition_variable cv_empty, cv_full;
//     int capacity_{};
//     bool destroyed_{false};
// public:
//     BBQueue(int capacity) : capacity_(capacity){}

//     void push(T item){
//         {
//             std::unique_lock<std::mutex> lock(mtx);
//             cv_full.wait(lock, [this](){
//                 return bbqueue.size() < capacity || destroyed_ ;
//             });

//             if(destroyed_)
//                 return;

//             bbqueue.push(item);
//         }
//         cv_empty.notify_one(); 
//     }

//     T pop(){
//         T t;
//         {
//             std::unique_lock<std::mutex> lock(mtx);
//             cv_empty.wait(lock, [this](){
//                 return !bbqueue.empty() || destroyed_;
//             });

//             if(destroyed_){
//                 return;
//             }

//             t = bbqueue.front();
//             bbqueue.pop();
//         }
//         cv_full.notify_one();
//         return t;
//     }

//     ~BBQueue(){
//         {
//             std::unique_lock<std::mutex> lock(mtx);
//             destroyed_ = true;
//         }
//         cv_full.notify_all();
//         cv_empty.notify_all();
//     }
// };



#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class BoundedBlockingQueue {
    std::queue<T> queue_;
    mutable std::mutex mtx_;                    // mutable → allows const member functions if needed
    std::condition_variable cv_not_full_, cv_not_empty_;
    const size_t capacity_;
    bool shutdown_ = false;

public:
    explicit BoundedBlockingQueue(size_t capacity) : capacity_(capacity) {}

    // Prevent copy/assignment
    BoundedBlockingQueue(const BoundedBlockingQueue&) = delete;
    BoundedBlockingQueue& operator=(const BoundedBlockingQueue&) = delete;

    // Producer with copy or move semantics
    void push(const T& item) {
        std::unique_lock lock(mtx_);
        cv_not_full_.wait(lock, [this] {
            return queue_.size() < capacity_ || shutdown_;
        });
        if (shutdown_) return;

        queue_.push(item);
        cv_not_empty_.notify_one();
    }

    void push(T&& item) {
        std::unique_lock lock(mtx_);
        cv_not_full_.wait(lock, [this] {
            return queue_.size() < capacity_ || shutdown_;
        });
        if (shutdown_) return;

        queue_.push(std::move(item));
        cv_not_empty_.notify_one();
    }

    // Avoid copies for construction → universal reference
    template<typename... Args>
    void emplace(Args&&... args) {
        std::unique_lock lock(mtx_);
        cv_not_full_.wait(lock, [this] {
            return queue_.size() < capacity_ || shutdown_;
        });
        if (shutdown_) return;

        queue_.emplace(std::forward<Args>(args)...);
        cv_not_empty_.notify_one();
    }

    [[nodiscard]] std::optional<T> pop() {
        std::unique_lock lock(mtx_);
        cv_not_empty_.wait(lock, [this] {
            return !queue_.empty() || shutdown_;
        });

        if (shutdown_ && queue_.empty()) return std::nullopt;

        T item = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return item;
    }

    void shutdown() {
        {
            std::lock_guard lock(mtx_);
            shutdown_ = true;
        }
        cv_not_full_.notify_all();
        cv_not_empty_.notify_all();
    }

    ~BoundedBlockingQueue() {
        shutdown();
    }
};
