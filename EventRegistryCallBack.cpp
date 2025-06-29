// Technical Round 1
// Topic: Threads and Synchronization
// Question: There are multiple users calling a method reg_cb at different intances of time, as shown below. Simultaneously, there is an event happening. 
// All the user requests that were made during the execution of the event should wait till the event completes and then execute the reg_cb method. 
// Once the event is finished, the user requests to the reg_cb method can be executed immediatly. Implement how to handle the given scenario.

// 					Event in progress
// ----|---------------|--------------------|-------------------|--------------> timeline
// U1: reg_cb(f1)     U2: reg_cb(f2)      Event completed      U3:reg_cb(f3)
// 									   (execute f1,f2)      (execute f3)
// Was asked many questions on basic fundamentals like,

// When does a concurrant modification exception occur?
// When is the possibility of same thread (user x) calling the reg_cb() twice?
// What are the possible deadlock scenarios?
// What is mutex? etc..


// - We can make a callback registry queue, also maintain some eventStarted atomic variable.

// - If eventStarted atomic variable is set, then all the reg_cb functions will be addded to this queue.

// And ocne eventStopped, notify all the threads that they can wake up and execute the registered callbacks.

// So we can implement a RequestScheduler, where it stores a callbakc registry and maintains this .

// ****************** FIrst version ****************

// How to schedule event call back.

class EventRegistry{

    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::function<void>> callbackRegistry;
    bool eventStarted{false};
    std::thread eventThread;
public:

    EventRegistry(){
        eventThread = std::thread([this](){
            while(true){
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this](){
                    return !eventStarted && !callbackRegistry.empty();
                });

                while(!callbackRegistry.empty()){
                    std::function<void> cb = callbackRegistry.front();
                    callbackRegistry.pop();
                    cb(); // may be execute in some different thread
                }
            }
        });
    }

    void registerCallback(std::function<void> cb){
        std::unique_lock<std::mutex> lock(mtx);
        if(eventStarted){
            callbackRegistry.push(cb);
        } else {
            cb();
        }
    }

    void setEventStarted(){
        std::unique_lock<std::mutex> lock(mtx);
        eventStarted = true;
    }

    void setEventStopped(){
        {
            std::unique_lock<std::mutex> lock(mtx);
            eventStarted = false;
        }
        cv.notify_one();
    }
};

// **************** Event Registry CallBack Imporved Version ***************

#include <iostream>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class EventRegistry {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> callbackQueue_;
    bool eventInProgress_{false};
    std::atomic<bool> stop_{false};
    std::thread eventThread_;

public:
    EventRegistry() {
        eventThread_ = std::thread([this]() {
            worker();
        });
    }

    // Register a callback
    void reg_cb(std::function<void()> cb) {
        std::unique_lock lock(mtx_);
        if (eventInProgress_) {
            callbackQueue_.push(cb);  // Store for later
        } else {
            lock.unlock();  // Unlock before running user code
            cb();           // Execute immediately
        }
    }

    // Mark the event as started
    void startEvent() {
        std::lock_guard lock(mtx_);
        eventInProgress_ = true;
    }

    // Mark the event as completed → triggers execution of waiting callbacks
    void stopEvent() {
        {
            std::lock_guard lock(mtx_);
            eventInProgress_ = false;
        }
        cv_.notify_one();
    }

    // Graceful shutdown
    void shutdown() {
        stop_ = true;
        cv_.notify_one();
        if (eventThread_.joinable()) eventThread_.join();
    }

    ~EventRegistry() {
        shutdown();
    }

private:
    void worker() {
        while (true) {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this]() {
                return !eventInProgress_ || stop_;
            });

            if (stop_) break;

            // Process all queued callbacks
            while (!callbackQueue_.empty()) {
                auto cb = callbackQueue_.front();
                callbackQueue_.pop();
                lock.unlock();  // Unlock mutex during execution of callback
                try {
                    cb();
                } catch (const std::exception& e) {
                    std::cerr << "Callback threw exception: " << e.what() << '\n';
                } catch (...) {
                    std::cerr << "Callback threw unknown exception\n";
                }
                lock.lock();
            }
        }
    }
};


// **************** With Thread Pool ***************




#include <iostream>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <future>

class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_{false};

public:
    explicit ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(mtx_);
                        cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "Task exception: " << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "Unknown task exception\n";
                    }
                }
            });
        }
    }

    void enqueue(std::function<void()> func) {
        {
            std::lock_guard lock(mtx_);
            tasks_.push(std::move(func));
        }
        cv_.notify_one();
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }
};

class EventRegistry {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> callbackQueue_;
    bool eventInProgress_{false};
    std::atomic<bool> stop_{false};
    std::thread monitorThread_;
    ThreadPool threadPool_;

public:
    explicit EventRegistry(size_t threadCount = std::thread::hardware_concurrency())
        : threadPool_(threadCount) {
        monitorThread_ = std::thread([this]() { monitor(); });
    }

    void reg_cb(std::function<void()> cb) {
        std::unique_lock lock(mtx_);
        if (eventInProgress_) {
            callbackQueue_.push(cb);
        } else {
            lock.unlock();
            threadPool_.enqueue(std::move(cb)); // Execute immediately in pool
        }
    }

    void startEvent() {
        std::lock_guard lock(mtx_);
        eventInProgress_ = true;
    }

    void stopEvent() {
        {
            std::lock_guard lock(mtx_);
            eventInProgress_ = false;
        }
        cv_.notify_one();
    }

    void shutdown() {
        stop_ = true;
        cv_.notify_one();
        if (monitorThread_.joinable()) monitorThread_.join();
        // threadPool_ destructor handles stopping its threads
    }

    ~EventRegistry() {
        shutdown();
    }

private:
    void monitor() {
        while (true) {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this]() {
                return !eventInProgress_ || stop_;
            });
            if (stop_) break;

            while (!callbackQueue_.empty()) {
                auto cb = callbackQueue_.front();
                callbackQueue_.pop();
                lock.unlock();
                threadPool_.enqueue(std::move(cb));
                lock.lock();
            }
        }
    }
};
