

class Singleton{

    static Singleton* instance_;
    Singleton() = default;
    std::mutex mtx;
public:
    Singleton* getInstance(){
        if(instance_ == nullptr){
            std::lock_guard<std::mutex> lock(mtx);
            if(instance_ == nullptr){
                instance_ = new Singleton();
            }
        }
        return instance_;
    }
};

Singleton::instance_ == nullptr;