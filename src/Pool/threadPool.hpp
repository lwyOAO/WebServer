#pragma once

#include <mutex>
#include <queue>
#include <thread>
#include <assert.h>
#include <functional>
#include <condition_variable>
#include "../Log/log.h"

class ThreadPool {
public:
    explicit ThreadPool(int threadCount = 8) : pool_(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        LOG_INFO(INS()) << "ThreadPool inited." << std::endl;
        for(int i = 0; i < threadCount; ++i) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> lck(pool->mtx_);
                while(true) {
                    if(!pool->tasks_.empty()) {
                        //move()直接移动对象，避免再构造一次对象
                        auto task = std::move(pool->tasks_.front());
                        pool->tasks_.pop();
                        lck.unlock();
                        LOG_DEBUG(INS()) << "exec one task" << std::endl;
                        task();
                        lck.lock();
                    }
                    else if(pool->isClosed_) {
                        LOG_INFO(INS()) << "threadpool close!" << std::endl;
                        break;
                    }
                    else {
                        pool->cond_.wait(lck);
                    }
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        pool_->isClosed_ = true;
    }
private:
    struct Pool {
        std::mutex mtx_;
        bool isClosed_;
        std::condition_variable cond_;
        std::queue<std::function<void()>> tasks_;
    };
    std::shared_ptr<Pool> pool_;
};

