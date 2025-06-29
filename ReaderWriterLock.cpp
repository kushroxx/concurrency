

class RWLock(){

    int data{};
    std::mutex mtx;
public:

    int readData(){
        std::shared_lock<std::mutex> lock(mtx);
        return data;
    }

    void writeData(int updatedData){
        std::unique_lock<std::mutex> lock(mtx);
        data = updatedData;
    }

}


class RWLock{

    std::mutex mtx;
    std::condition_variable cv;
    int active_readers{};
    int active_writers{};
public:
    void lockRead(){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this](){
            return active_writers == 0;
        });
        active_readers++;
    }

    void unlockRead(){
        std::unique_lock<std::mutex> lock(mtx);
        active_readers--;
        if(active_readers == 0)
            cv.notify_all();
    }

    void lockWrite(){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this](){
            return active_readers == 0 && active_writers == 0;
        });
        active_writers++;
    }

    void unlockWrite(){
        std::unique_lock<std::mutex> lock(mtx);
        active_writers--;
        cv.notify_all();
    }
}