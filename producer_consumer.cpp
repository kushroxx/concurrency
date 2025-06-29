

// ✅ Q1: What problem does the producer-consumer pattern solve?
// Your Answer:
// Basically it solves the synchronization issue when two threads are trying to write/read some resource. One thread is updating some variable and the other thread needs to fetch the updated state.

// Refined Answer:
// The producer-consumer pattern solves the problem of coordinating work between threads that produce data (producer) and threads that consume data (consumer), especially when they operate at different speeds. It ensures:

// Thread-safe communication via shared data structures (e.g., queues)

// Synchronization so consumers wait for data and producers wait for space (if bounded)

// Avoids race conditions, busy-waiting, and lost updates

// ✅ Q2: Why do we need a mutex in this setup?
// Your Answer:
// For guarding the tasks queue as it is accessed by both the threads and to prevent data race.

// Perfect. This is concise and correct. A mutex ensures mutual exclusion, so only one thread can access or modify the queue at a time — which avoids data races and corruption.

// ✅ Q3: What happens if the producer produces faster than the consumer can consume?
// Your Answer:
// There will be no issue as producer can produce at faster rate but consumer will consume all the elements present in the queue.

// Refined Answer (Important nuance):
// ✅ This is true if the queue is unbounded — the consumer will eventually catch up.
// ❗However, in real-world systems, queues are often bounded (limited in size) to avoid:

// Out-of-memory issues

// Unbounded resource usage

// So if the producer is faster:

// With unbounded queue → backlog grows, but no crash.

// With bounded queue → producer must wait (block) when the queue is full → this needs extra logic in code (e.g., additional cv.wait() in producer).

// ------------------------xxxxxxxxxxxx--------------------------


// ⚙️ Level 2: Intermediate Concepts
// What are spurious wakeups and how do you handle them correctly?

// Why do we use cv.wait(lock, predicate) instead of just cv.wait(lock)?

// What could go wrong if you call notify_one() before the consumer calls wait()?

// 🔐 Level 3: Synchronization Details
// What's the difference between using:

// std::condition_variable

// Busy waiting with a while loop and sleep?

// Can notify_all() be used instead of notify_one()? What are the trade-offs?

// How would you gracefully shutdown the consumer thread after the producer is done?

// 🧠 Level 4: Code/Design Questions
// What data structure would you use for the task queue to make it thread-safe?

// What if you had multiple producers and multiple consumers?

// What changes would you make?

// How would you avoid data races?

// Can you design a bounded buffer (fixed-size queue)? How would you make producers wait when it’s full?

// 🚀 Level 5: Performance & Edge Cases
// What are the implications of holding a lock for too long?

// How would you improve throughput if tasks take time to process?

// How would you detect and handle deadlocks or livelocks in this scenario?

#include <iostream>
#include <thread>

std::queue<int> tasks;
std::mutex m;
std::condition_variable cv1;
std::condition_variable cv2;
bool done = false;
int MAX_SIZE = 10;

std::mutex cout_mutex;

void log(const std::string& message) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << message << std::endl;
}

void consumer(){

    while(true){
        std::unique_lock<std::mutex> lock(m);
        //std::cout << " Consumer Acquired Lock " << std::endl;
        log(" Consumer Acquired Lock ");
        cv1.wait(lock, []{ 
            //std::cout << " Consumer Waiting " << std::endl;
            log(" Consumer Waiting ");
            return !tasks.empty() || done;
        });
        //std::cout << " Waking Up " << std::endl;
        while(!tasks.empty()){
            int val = tasks.front();
            tasks.pop();
            //std::cout << "Consumed Value : " << val << std::endl;
            log(" Consumed Value ");
            cv2.notify_one();
        }
        if(done)
            break;
    }
}

void producer(){
    
    for(int i = 0; i < 100; i++){
        {
            std::unique_lock<std::mutex> lock(m);

            cv2.wait(lock, []{
                std::cout << " Waiitng for tasks queue" << std::endl;
                return tasks.size() != MAX_SIZE;
            });

            std::cout << " Queue Size " << tasks.size() << std::endl; 
            tasks.push(i);
            //std::cout << "Pushed Value " << i << std::endl;
            //std::cout << " Producer Acquired Lock " << std::endl;
            log(" Pushed Value ");
        }
        //std::cout << "Notified Value" << std::endl;
        cv1.notify_one();
    }

    {
        std::lock_guard<std::mutex> lock(m);
        done = true;
    }

    cv1.notify_all();

}

int main() {

    std::thread prod(producer);
    std::thread cons(consumer);

    if(prod.joinable())
        prod.join();

    if(cons.joinable())
        cons.join();

        
    return 0;

}