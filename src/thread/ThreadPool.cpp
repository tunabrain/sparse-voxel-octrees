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

#include "ThreadPool.hpp"

#include <chrono>

ThreadPool::ThreadPool(uint32 threadCount)
: _threadCount(threadCount),
  _terminateFlag(false)
{
    startThreads();
}

ThreadPool::~ThreadPool()
{
    stop();
}

std::shared_ptr<TaskGroup> ThreadPool::acquireTask(uint32 &subTaskId)
{
    if (_terminateFlag)
        return nullptr;
    std::shared_ptr<TaskGroup> task = _tasks.front();
    if (task->isAborting()) {
        _tasks.pop_front();
        return nullptr;
    }
    subTaskId = task->startSubTask();
    if (subTaskId == task->numSubTasks() - 1)
        _tasks.pop_front();
    return std::move(task);
}

void ThreadPool::runWorker(uint32 threadId)
{
    while (!_terminateFlag) {
        uint32 subTaskId;
        std::shared_ptr<TaskGroup> task;
        {
            std::unique_lock<std::mutex> lock(_taskMutex);
            _taskCond.wait(lock, [this]{return _terminateFlag || !_tasks.empty();});
            task = acquireTask(subTaskId);
        }
        if (task)
            task->run(threadId, subTaskId);
    }
}

void ThreadPool::startThreads()
{
    _terminateFlag = false;
    for (uint32 i = 0; i < _threadCount; ++i) {
        _workers.emplace_back(new std::thread(&ThreadPool::runWorker, this, uint32(_workers.size())));
        _idToNumericId.insert(std::make_pair(_workers.back()->get_id(), i));
    }
}

void ThreadPool::yield(TaskGroup &wait)
{
    std::chrono::milliseconds waitSpan(10);
    uint32 id = _threadCount; // Threads not in the pool get a previously unassigned id

    auto iter = _idToNumericId.find(std::this_thread::get_id());
    if (iter != _idToNumericId.end())
        id = iter->second;

    while (!wait.isDone() && !_terminateFlag) {
        uint32 subTaskId;
        std::shared_ptr<TaskGroup> task;
        {
            std::unique_lock<std::mutex> lock(_taskMutex);
            if (!_taskCond.wait_for(lock, waitSpan, [this]{return _terminateFlag || !_tasks.empty();}))
                continue;
            task = acquireTask(subTaskId);
        }
        if (task)
            task->run(id, subTaskId);
    }
}

void ThreadPool::reset()
{
    stop();
    _tasks.clear();
    startThreads();
}

void ThreadPool::stop()
{
    _terminateFlag = true;
    {
        std::unique_lock<std::mutex> lock(_taskMutex);
        _taskCond.notify_all();
    }
    while (!_workers.empty()) {
        _workers.back()->detach();
        _workers.pop_back();
    }
}

std::shared_ptr<TaskGroup> ThreadPool::enqueue(TaskFunc func, int numSubtasks, Finisher finisher)
{
    std::shared_ptr<TaskGroup> task(std::make_shared<TaskGroup>(std::move(func),
            std::move(finisher), numSubtasks));

    {
        std::unique_lock<std::mutex> lock(_taskMutex);
        _tasks.emplace_back(task);
        if (numSubtasks == 1)
            _taskCond.notify_one();
        else
            _taskCond.notify_all();
    }

    return std::move(task);
}
