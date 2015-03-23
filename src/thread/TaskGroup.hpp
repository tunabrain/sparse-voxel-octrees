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

#ifndef TASKGROUP_HPP_
#define TASKGROUP_HPP_

#include "IntTypes.hpp"

#include <condition_variable>
#include <functional>
#include <exception>
#include <stdexcept>
#include <atomic>
#include <mutex>

class TaskGroup
{
    typedef std::function<void(uint32, uint32, uint32)> TaskFunc;
    typedef std::function<void()> Finisher;

    TaskFunc _func;
    Finisher _finisher;

    std::exception_ptr _exceptionPtr;
    std::atomic<uint32> _startedSubTasks;
    std::atomic<uint32> _finishedSubTasks;
    uint32 _numSubTasks;

    std::mutex _waitMutex;
    std::condition_variable _waitCond;
    std::atomic<bool> _done, _abort;

    void finish()
    {
        if (_finisher && !_abort)
            _finisher();

        _done = true;
        _waitCond.notify_all();
    }

public:
    TaskGroup(TaskFunc func, Finisher finisher, uint32 numSubTasks)
    : _func(std::move(func)),
      _finisher(std::move(finisher)),
      _startedSubTasks(0),
      _finishedSubTasks(0),
      _numSubTasks(numSubTasks),
      _done(false),
      _abort(false)
    {
    }

    void run(uint32 threadId, uint32 taskId)
    {
        try {
            _func(taskId, _numSubTasks, threadId);
        } catch (...) {
            _exceptionPtr = std::current_exception();
        }

        uint32 num = ++_finishedSubTasks;
        if (num == _numSubTasks || (_abort && num == _startedSubTasks))
            finish();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(_waitMutex);
        _waitCond.wait(lock, [this]{return _done == true;});

        if (_exceptionPtr)
            std::rethrow_exception(_exceptionPtr);
    }

    void abort()
    {
        _abort = true;
        _done = _done || _startedSubTasks == 0;
    }

    bool isAborting() const
    {
        return _abort;
    }

    bool isDone() const
    {
        return _done;
    }

    uint32 startSubTask()
    {
        return _startedSubTasks++;
    }

    uint32 numSubTasks() const
    {
        return _numSubTasks;
    }
};

#endif /* TASKGROUP_HPP_ */
