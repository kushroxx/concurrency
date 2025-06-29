
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>


// ✅ ThreadPool Interview Questions with Answers
// 1️⃣ What is a ThreadPool?
// A ThreadPool is a pool of pre-created worker threads used to execute multiple tasks concurrently, without the overhead of constantly creating and destroying threads.

// Benefits:

// Reduces thread creation overhead.

// Controls concurrency.

// Enables reuse of threads for multiple tasks.

// 2️⃣ Why is ThreadPool preferred over creating threads manually?
// Avoids the overhead of frequent thread creation and destruction.

// Prevents resource exhaustion by limiting concurrency.

// Can support task queuing and load balancing.

// 3️⃣ How does your ThreadPool work?
// On creation, it spawns N worker threads.

// Tasks are stored in a queue protected by a mutex.

// Worker threads wait on a condition_variable.

// When a task is enqueued, one thread wakes up and executes it.

// Supports graceful shutdown to avoid leaving tasks unprocessed.

// 4️⃣ How does enqueue() work?
// Accepts a callable (std::function<void()> or generic with template).

// Acquires a lock on the queue, pushes the task, and calls notify_one() on the condition variable.

// 5️⃣ What happens if stop() is never called before destruction?
// My implementation calls stop() in the destructor, ensuring cleanup happens even if the user forgets.

// 6️⃣ Why is stop_ variable needed?
// It’s used to:

// Signal worker threads to exit after pending work is done.

// Prevent enqueueing new tasks after shutdown has started.

// 7️⃣ What happens if enqueue() is called after stop()?
// Ideally: Check the stop_ flag inside enqueue() and throw an exception or ignore the task.

// Example:

// cpp
// Copy
// Edit
// if (stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
// 8️⃣ What happens if a task throws an exception?
// If the task throws and it’s not caught, the program may terminate.

// Solution: Wrap task execution in try-catch:

// cpp
// Copy
// Edit
// try {
//     task();
// } catch (const std::exception& e) {
//     std::cerr << "Task exception: " << e.what() << std::endl;
// }
// 9️⃣ Can you return values from tasks?
// Yes, by modifying enqueue() to return std::future<T> using std::packaged_task or std::promise.

// This allows the caller to retrieve the result asynchronously.

// 🔟 How do you handle multiple producers/consumers?
// Multiple producers: All can safely enqueue tasks using a mutex.

// Multiple consumers (threads): Each worker thread safely dequeues tasks using the mutex + condition variable.

// 1️⃣1️⃣ notify_one() vs notify_all()?
// notify_one() wakes one waiting thread → Efficient when one task = one thread.

// notify_all() wakes all → Useful if all threads might have work (or when shutting down).

// 1️⃣2️⃣ Is your ThreadPool copyable/movable?
// Should NOT be copyable (because of threads, mutexes, and condition variables).

// It can be movable but usually better to delete both copy/move to avoid mistakes.

// cpp
// Copy
// Edit
// ThreadPool(const ThreadPool&) = delete;
// ThreadPool& operator=(const ThreadPool&) = delete;
// 1️⃣3️⃣ What’s the Rule of 3/5/0 in C++ related to ThreadPool?
// Destructor, Copy constructor, Copy assignment → Rule of 3.

// Add Move constructor and Move assignment → Rule of 5.

// In this case, best to use Rule of 3 and delete copy/move unless you specifically implement them.

// 1️⃣4️⃣ What about pending tasks on shutdown?
// Can provide an option in stop() like:

// cpp
// Copy
// Edit
// void stop(bool executeRemaining = false);
// If executeRemaining == true, let threads finish all tasks before joining.

// Otherwise, discard pending tasks safely.

// 1️⃣5️⃣ How does this compare with std::async?
// std::async manages threads for you → simpler API.

// ThreadPool → more control over concurrency, better for high-load systems, reusing threads, or implementing futures/promises in a managed way.

// 1️⃣6️⃣ How can you implement priority-based task execution?
// Use a priority queue (std::priority_queue) instead of a regular queue.

// Tasks should then be comparable or wrapped in a structure that supports priority comparison.

// 1️⃣7️⃣ Can you scale ThreadPool dynamically?
// Yes → Add an addThread() function to increase pool size.

// Requires additional synchronization to manage thread lifecycles.



// template<typename F, typename... Args>
// auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
//     using return_type = decltype(f(args...));

//     auto task = std::make_shared<std::packaged_task<return_type()>>(
//         std::bind(std::forward<F>(f), std::forward<Args>(args)...)
//     );

//     std::future<return_type> res = task->get_future();

//     {
//         std::lock_guard<std::mutex> lock(queue_mutex_);
//         tasks_queue_.emplace([task](){ (*task)(); });
//     }
//     queue_cv_.notify_one();
//     return res;
// }

class ThreadPool{
public:
    explicit ThreadPool(int num_threads){
        stop_ = false;
        for(int i = 0; i < num_threads; i++){
            threads_.emplace_back(std::thread([this]{
                while(true){

                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        queue_cv_.wait(lock, [this]{
                            return !tasks_queue_.empty() || stop_;
                        });

                        if(stop_ && tasks_queue_.empty())
                            return;

                        task = tasks_queue_.front();
                        tasks_queue_.pop();
                    }

                    task();
                }
            }));
        }
    }

    void enqueue(std::function<void()> task){
        if(!stop_){
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                tasks_queue_.push(std::move(task));
            }
            queue_cv_.notify_one();
        }
    }

    void stop(){
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }

        queue_cv_.notify_all();

        for(auto &t : threads_){
            if(t.joinable())
                t.join();
        }
    }

    ~ThreadPool(){
        stop();
    }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    bool stop_;
};

void printFunc(int counter){
    std::cout << " Printing Function : " << counter << std::endl;
}

int main(){

    ThreadPool pool(10);

    for(int i = 0; i < 100; i++){

        pool.enqueue(std::bind(printFunc, i));
    }

    pool.stop();
}
// #include <vector>
// #include <queue>
// #include <thread>
// #include <future>
// #include <functional>
// #include <mutex>
// #include <condition_variable>
// #include <atomic>

// class ThreadPool {
// public:
//     explicit ThreadPool(size_t num_threads) : stop_(false) {
//         for (size_t i = 0; i < num_threads; ++i) {
//             threads_.emplace_back([this] {
//                 while (true) {
//                     std::function<void()> task;
//                     {
//                         std::unique_lock<std::mutex> lock(queue_mutex_);
//                         cv_.wait(lock, [this] {
//                             return stop_ || !tasks_.empty();
//                         });

//                         if (stop_ && tasks_.empty())
//                             return;

//                         task = std::move(tasks_.front());
//                         tasks_.pop();
//                     }
//                     task();
//                 }
//             });
//         }
//     }

//     ~ThreadPool() {
//         stop();
//     }

//     // Disable copy semantics
//     ThreadPool(const ThreadPool&) = delete;
//     ThreadPool& operator=(const ThreadPool&) = delete;

//     template<typename F, typename... Args>
//     auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
//         using return_type = std::invoke_result_t<F, Args...>;

//         auto task = std::make_shared<std::packaged_task<return_type()>>(
//             std::bind(std::forward<F>(f), std::forward<Args>(args)...)
//         );

//         std::future<return_type> result = task->get_future();
//         {
//             std::lock_guard<std::mutex> lock(queue_mutex_);
//             if (stop_)
//                 throw std::runtime_error("Enqueue on stopped ThreadPool");

//             tasks_.emplace([task]() { (*task)(); });
//         }
//         cv_.notify_one();
//         return result;
//     }

//     void stop() {
//         {
//             std::lock_guard<std::mutex> lock(queue_mutex_);
//             stop_ = true;
//         }
//         cv_.notify_all();
//         for (std::thread &worker : threads_) {
//             if (worker.joinable())
//                 worker.join();
//         }
//     }

// private:
//     std::vector<std::thread> threads_;
//     std::queue<std::function<void()>> tasks_;
//     std::mutex queue_mutex_;
//     std::condition_variable cv_;
//     bool stop_;
// };
