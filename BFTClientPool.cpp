#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

#include <queue>
#include <memory>
#include <functional>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads) : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i)
            threads_.emplace_back([this]() { worker(); });
    }

    ~ThreadPool() { shutdown(); }

    void submit(std::function<void()> job) {
        {
            std::lock_guard lock(mtx_);
            tasks_.push(std::move(job));
        }
        cv_.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : threads_) if (t.joinable()) t.join();
    }

private:
    void worker() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock lock(mtx_);
                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                job = std::move(tasks_.front());
                tasks_.pop();
            }
            job();
        }
    }

    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_;
};

class BFTClient {
public:
    explicit BFTClient(int id) : id_(id) {}
    void sendRequest(const std::string& req) {
        std::cout << "[Client " << id_ << "] Processing: " << req << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
private:
    int id_;
};

using ClientPtr = std::shared_ptr<BFTClient>;

struct RequestJob {
    std::string request;
    std::function<void()> callback;
};

class BFTClientPool {
public:
    BFTClientPool(size_t numClients, size_t maxPending, std::shared_ptr<ThreadPool> threadPool)
        : threadPool_(threadPool), maxPending_(maxPending) {
        for (size_t i = 0; i < numClients; ++i)
            clients_.push(std::make_shared<BFTClient>(i + 1));
    }

    bool sendRequest(const std::string& req, std::function<void()> callback = {}) {
        ClientPtr client;
        {
            std::unique_lock lock(mtx_);
            if (!clients_.empty()) {
                client = clients_.front();
                clients_.pop();
            } else {
                if (pending_.size() >= maxPending_) {
                    std::cout << "Queue Full → rejecting: " << req << "\n";
                    return false;
                }
                pending_.push(RequestJob{req, callback});
                return true;
            }
        }

        // Submit work immediately if client was available
        threadPool_->submit([this, client, req, callback]() {
            client->sendRequest(req);
            if (callback) callback();
            {
                std::lock_guard lock(mtx_);
                clients_.push(client);
            }
            drainPendingQueue();
        });
        return true;
    }

private:
    void drainPendingQueue() {
        ClientPtr client;
        RequestJob pendingJob;
        {
            std::lock_guard lock(mtx_);
            if (pending_.empty() || clients_.empty()) return;
            pendingJob = pending_.front();
            pending_.pop();
            client = clients_.front();
            clients_.pop();
        }

        threadPool_->submit([this, client, pendingJob]() {
            client->sendRequest(pendingJob.request);
            if (pendingJob.callback) pendingJob.callback();
            {
                std::lock_guard lock(mtx_);
                clients_.push(client);
            }
            drainPendingQueue();  // Recursively try to drain more
        });
    }

    std::queue<ClientPtr> clients_;
    std::queue<RequestJob> pending_;
    std::mutex mtx_;
    const size_t maxPending_;
    std::shared_ptr<ThreadPool> threadPool_;
};


int main() {
    auto threadPool = std::make_shared<ThreadPool>(4);
    BFTClientPool clientPool(3, 5, threadPool);

    for (int i = 0; i < 15; ++i) {
        bool accepted = clientPool.sendRequest("Request #" + std::to_string(i), [i]() {
            std::cout << "Callback → Done with Request #" << i << "\n";
        });
        if (!accepted) {
            std::cout << "Rejected → Request #" << i << "\n";
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    threadPool->shutdown();
}
