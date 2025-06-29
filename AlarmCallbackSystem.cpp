// How do u design a alarm callback system? It can take a callback function and a time after which to call this function.
//  Your system has to keep track of everything and do this in a timely and performant manner. 

struct Alarm{
    std::function<void> cb;
    int callback_time;
    int pushed_time;
};

class ThreadPool{

};


class AlarmCallbackSystem{

    std::queue<Alarm> callback_registry;
    std::mutex mtx;
    std::condition_variable cv;
    ThreadPool threadPool;
    std::thread alarm_thread;

    void loop(){
        while(true){
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [](){
                    return !callback_registry.empty();
                });

                while(!callback_registry.empty()){
                    std::function<void> cb;
                    Alarm item = callback_registry.front();

                    if(item.pushed - current_time > callback_time){
                        callback_registry.pop();
                        threadPool.enqueue(std::move(item.cb));
                    }
                }
            }
        }
    }
public:
    AlarmCallbackSystem(){
        alarm_thread = std::thread(loop);
    }

    void pushEvent(std::function<void> cb, int callback_time){
        {
            int current_time = 1;
            std::unique_lock<std::mutex> lock(mtx);

            Alarm alarm(cb, callback_time,current_time);
            callback_registry.push(alarm);
        }

        cv.notify_one();
    }


};


struct Alarm {
    std::function<void()> cb;
    std::chrono::steady_clock::time_point scheduled_time;

    bool operator>(const Alarm& other) const {
        return scheduled_time > other.scheduled_time;
    }
};


class AlarmCallbackSystem {
private:
    std::priority_queue<Alarm, std::vector<Alarm>, std::greater<Alarm>> callback_registry;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stopped = false;
    std::thread alarm_thread;
    ThreadPool threadPool;

    void loop() {
        std::unique_lock<std::mutex> lock(mtx);
        while (!stopped) {
            if (callback_registry.empty()) {
                cv.wait(lock);
            } else {
                auto next_alarm = callback_registry.top();
                auto now = std::chrono::steady_clock::now();
                if (now >= next_alarm.scheduled_time) {
                    callback_registry.pop();
                    lock.unlock();
                    threadPool.enqueue(next_alarm.cb);
                    lock.lock();
                } else {
                    cv.wait_until(lock, next_alarm.scheduled_time);
                }
            }
        }
    }

public:
    AlarmCallbackSystem() {
        alarm_thread = std::thread(&AlarmCallbackSystem::loop, this);
    }

    void pushEvent(std::function<void()> cb, std::chrono::milliseconds delay) {
        auto scheduled_time = std::chrono::steady_clock::now() + delay;
        {
            std::lock_guard<std::mutex> lock(mtx);
            callback_registry.push(Alarm{ cb, scheduled_time });
        }
        cv.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stopped = true;
        }
        cv.notify_all();
        if (alarm_thread.joinable()) alarm_thread.join();
    }
};
