// Coding busy waiting mutex


class BWMutex{

    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock(){
        while(flag.test_and_set(std::memory_order_acquire)){

        }
    }

    void unlock(){
        flag.clear(std::memory_order_release);
    }

};



class SpinLock{

    std::atomic<bool> locked{false};
public:
    void lock(){
        bool expected = false;
        while(!locked.compare_exchange_weak(expected, true, std::memory_order_acquire)){

        }
    }

    void unlock(){
        locked.store(false, std::memory_order_release);
    }

};