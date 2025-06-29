#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <list>
#include <optional>
#include <functional>
#include <random>
#include <chrono>
#include <thread>

#define BUCKET_SIZE 16

template <typename K, typename V>
class ConcurrentMap {

    struct BucketEntry {
        K key;
        V value;
    };

    size_t bucket_size_{};

    size_t getHash(const K& key) const {
        return std::hash<K>{}(key) % bucket_size_;
    }

    mutable std::vector<std::shared_mutex> mtxList_;
    std::vector<std::list<BucketEntry>> buckets_;

public:
    ConcurrentMap() : bucket_size_(BUCKET_SIZE), buckets_(BUCKET_SIZE), mtxList_(BUCKET_SIZE) {}

    void insert(const K& key, const V& value) {
        size_t hash = getHash(key);
        auto& bucket = buckets_[hash];
        std::unique_lock<std::shared_mutex> lock(mtxList_[hash]);
        for (auto& entry : bucket) {
            if (entry.key == key) {
                entry.value = value;
                return;
            }
        }
        bucket.push_back({key, value});
    }

    std::optional<V> get(const K& key) const {
        size_t hash = getHash(key);
        const auto& bucket = buckets_[hash];
        std::shared_lock<std::shared_mutex> lock(mtxList_[hash]);
        for (const auto& entry : bucket) {
            if (entry.key == key) {
                return entry.value;
            }
        }
        return std::nullopt;
    }
};

template <typename K, typename V>
class ConcurrentShardMap {

    std::vector<ConcurrentMap<K, V>> shards_;
    size_t shards_size_{};

    size_t getShardIndex(const K& key) const {
        return std::hash<K>{}(key) % shards_size_;
    }

public:
    ConcurrentShardMap(size_t shards) : shards_size_(shards), shards_(shards) {}

    void insert(const K& key, const V& value) {
        std::cout << " Inserting in Shard " << getShardIndex(key) << std::endl;
        shards_[getShardIndex(key)].insert(key, value);
    }

    std::optional<V> get(const K& key) {
        std::cout << " Found in Shard " << getShardIndex(key) << std::endl;
        return shards_[getShardIndex(key)].get(key);
    }
};

int main() {
    ConcurrentShardMap<int, int> map(5);

    constexpr int numThreads = 10;
    constexpr int numOpsPerThread = 1000;

    auto inserter = [&](int thread_id) {
        for (int i = 0; i < numOpsPerThread; ++i) {
            int key = thread_id * numOpsPerThread + i;
            map.insert(key, key * 10);
        }
    };

    auto getter = [&](int thread_id) {
        for (int i = 0; i < numOpsPerThread; ++i) {
            int key = thread_id * numOpsPerThread + i;
            auto result = map.get(key);
            if (result) {
                // Uncomment to see individual gets (may be noisy)
                // std::cout << "Thread " << thread_id << " got " << key << " : " << *result << '\n';
            }
        }
    };

    std::vector<std::thread> threads;

    // Start inserter threads
    for (int i = 0; i < numThreads / 2; ++i) {
        threads.emplace_back(inserter, i);
    }

    // Start getter threads
    for (int i = 0; i < numThreads / 2; ++i) {
        threads.emplace_back(getter, i);
    }

    for (auto& th : threads) {
        th.join();
    }

    std::cout << "All threads completed.\n";

    // Final check for a few known keys
    for (int i = 0; i < numThreads / 2; ++i) {
        int key = i * numOpsPerThread;
        auto result = map.get(key);
        if (result) {
            std::cout << "Final check: key = " << key << ", value = " << *result << '\n';
        }
    }

    return 0;
}
