#ifndef SIMULATION_OBJECT_POOL_H
#define SIMULATION_OBJECT_POOL_H
#include <memory>
#include <vector>

namespace simulation {
    template <typename T>
    class SharedObjectPool {
        std::vector<T*> pool;
    public:
        ~SharedObjectPool() {
            for (auto *p : pool) {
                delete p;
            }
        }

        std::shared_ptr<T> acquire() {
            T* raw_ptr;
            if (pool.empty()) raw_ptr = new T();
            else {
                raw_ptr = pool.back();
                pool.pop_back();
            }

            raw_ptr->reset();

            return std::shared_ptr<T>(raw_ptr, [this](T* ptr){
                this->pool.push_back(ptr);
            });
        }
    };

    template <typename T>
    class UniqueObjectPool {
        std::vector<T*> pool;
    public:
        ~UniqueObjectPool() {
            for (auto *p : pool) {
                delete p;
            }
        }

        std::unique_ptr<T> acquire() {
            T* raw_ptr;
            if (pool.empty()) raw_ptr = new T();
            else {
                raw_ptr = pool.back();
                pool.pop_back();
            }

            raw_ptr->reset();

            return std::unique_ptr<T>(raw_ptr, [this](T* ptr){
                this->pool.push_back(ptr);
            });
        }
    };
}

#endif // SIMULATION_OBJECT_POOL_H
