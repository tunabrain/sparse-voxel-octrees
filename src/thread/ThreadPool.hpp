/*
Copyright (c) 2013 Benedikt Bitterli

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include "TaskGroup.hpp"

#include "IntTypes.hpp"

#include <condition_variable>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>

class ThreadPool
{
    typedef std::function<void(uint32, uint32, uint32)> TaskFunc;
    typedef std::function<void()> Finisher;

    uint32 _threadCount;
    std::vector<std::unique_ptr<std::thread>> _workers;
    std::atomic<bool> _terminateFlag;

    std::deque<std::shared_ptr<TaskGroup>> _tasks;
    std::mutex _taskMutex;
    std::condition_variable _taskCond;

    std::unordered_map<std::thread::id, uint32> _idToNumericId;

    std::shared_ptr<TaskGroup> acquireTask(uint32 &subTaskId);
    void runWorker(uint32 threadId);
    void startThreads();

public:
    ThreadPool(uint32 threadCount);
    ~ThreadPool();

    void yield(TaskGroup &wait);

    void reset();
    void stop();

    std::shared_ptr<TaskGroup> enqueue(TaskFunc func, int numSubtasks = 1,
            Finisher finisher = Finisher());

    uint32 threadCount() const
    {
        return _threadCount;
    }
};

#endif /* THREADPOOL_HPP_ */
