// Copyright 2023 The turbo Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "turbo/network/poller/select_wrap.h"
#include "turbo/network/poller/event_poller.h"
#include "turbo/network/util/util.h"
#include "turbo/network/util/uv_errno.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/util/notice_center.h"
#include "turbo/network/network/sock_util.h"


#if defined(HAS_EPOLL)
#include <sys/epoll.h>

#if !defined(EPOLLEXCLUSIVE)
#define EPOLLEXCLUSIVE 0
#endif

#define EPOLL_SIZE 1024

//防止epoll惊群
#ifndef EPOLLEXCLUSIVE
#define EPOLLEXCLUSIVE 0
#endif

#define toEpoll(event)        (((event) & Event_Read)  ? EPOLLIN : 0) \
                            | (((event) & Event_Write) ? EPOLLOUT : 0) \
                            | (((event) & Event_Error) ? (EPOLLHUP | EPOLLERR) : 0) \
                            | (((event) & Event_LT)    ? 0 : EPOLLET)

#define toPoller(epoll_event)     (((epoll_event) & EPOLLIN) ? Event_Read   : 0) \
                                | (((epoll_event) & EPOLLOUT) ? Event_Write : 0) \
                                | (((epoll_event) & EPOLLHUP) ? Event_Error : 0) \
                                | (((epoll_event) & EPOLLERR) ? Event_Error : 0)
#endif //HAS_EPOLL

using namespace std;

namespace turbo {

EventPoller &EventPoller::Instance() {
    return *(EventPollerPool::Instance().getFirstPoller());
}

void EventPoller::addEventPipe() {
    SockUtil::setNoBlocked(_pipe.readFD());
    SockUtil::setNoBlocked(_pipe.writeFD());

    // 添加内部管道事件
    if (addEvent(_pipe.readFD(), EventPoller::Event_Read, [this](int event) { onPipeEvent(); }) == -1) {
        throw std::runtime_error("Add pipe fd to poller failed");
    }
}

EventPoller::EventPoller(std::string name) {
    _name = std::move(name);
#if defined(HAS_EPOLL)
    _epoll_fd = epoll_create(EPOLL_SIZE);
    if (_epoll_fd == -1) {
        throw runtime_error(StrPrinter << "Create epoll fd failed: " << get_uv_errmsg());
    }
    SockUtil::setCloExec(_epoll_fd);
#endif //HAS_EPOLL
    _logger = Logger::Instance().shared_from_this();
    addEventPipe();
}

void EventPoller::shutdown() {
    async_l([]() {
        throw ExitException();
    }, false, true);

    if (_loop_thread) {
        //防止作为子进程时崩溃
        try { _loop_thread->join(); } catch (...) {}
        delete _loop_thread;
        _loop_thread = nullptr;
    }
}

EventPoller::~EventPoller() {
    shutdown();
#if defined(HAS_EPOLL)
    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _epoll_fd = -1;
    }
#endif //defined(HAS_EPOLL)
    //退出前清理管道中的数据
    onPipeEvent();
    InfoL << this;
}

int EventPoller::addEvent(int fd, int event, PollEventCB cb) {
    TimeTicker();
    if (!cb) {
        WarnL << "PollEventCB is empty";
        return -1;
    }

    if (isCurrentThread()) {
#if defined(HAS_EPOLL)
        struct epoll_event ev = {0};
        ev.events = (toEpoll(event)) | EPOLLEXCLUSIVE;
        ev.data.fd = fd;
        int ret = epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
        if (ret == 0) {
            _event_map.emplace(fd, std::make_shared<PollEventCB>(std::move(cb)));
        }
        return ret;
#else
#ifndef _WIN32
        //win32平台，socket套接字不等于文件描述符，所以可能不适用这个限制
        if (fd >= FD_SETSIZE || _event_map.size() >= FD_SETSIZE) {
            WarnL << "select() can not watch fd bigger than " << FD_SETSIZE;
            return -1;
        }
#endif
        auto record = std::make_shared<Poll_Record>();
        record->fd = fd;
        record->event = event;
        record->call_back = std::move(cb);
        _event_map.emplace(fd, record);
        return 0;
#endif //HAS_EPOLL
    }

    async([this, fd, event, cb]() mutable {
        addEvent(fd, event, std::move(cb));
    });
    return 0;
}

int EventPoller::delEvent(int fd, PollCompleteCB cb) {
    TimeTicker();
    if (!cb) {
        cb = [](bool success) {};
    }

    if (isCurrentThread()) {
#if defined(HAS_EPOLL)
        bool success = epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == 0 && _event_map.erase(fd) > 0;
        if (success) {
            _event_cache_expired_map[fd] = true;
        }
        cb(success);
        return success ? 0 : -1;
#else
        bool success = _event_map.erase(fd);
        if (success) {
            _event_cache_expired_map[fd] = true;
        }
        cb(success);
        return 0;
#endif //HAS_EPOLL
    }

    //跨线程操作
    async([this, fd, cb]() mutable {
        delEvent(fd, std::move(cb));
    });
    return 0;
}

int EventPoller::modifyEvent(int fd, int event, PollCompleteCB cb) {
    TimeTicker();
    if (!cb) {
        cb = [](bool success) {};
    }
    if (isCurrentThread()) {
#if defined(HAS_EPOLL)
        struct epoll_event ev = { 0 };
        ev.events = toEpoll(event);
        ev.data.fd = fd;
        auto ret = epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
        cb(ret == 0);
        return ret;
#else
        auto it = _event_map.find(fd);
        if (it != _event_map.end()) {
            it->second->event = event;
        }
        cb(it != _event_map.end());
        return 0;
#endif // HAS_EPOLL
    }
    async([this, fd, event, cb]() mutable {
        modifyEvent(fd, event, std::move(cb));
    });
    return 0;
}

Task::Ptr EventPoller::async(TaskIn task, bool may_sync) {
    return async_l(std::move(task), may_sync, false);
}

Task::Ptr EventPoller::async_first(TaskIn task, bool may_sync) {
    return async_l(std::move(task), may_sync, true);
}

Task::Ptr EventPoller::async_l(TaskIn task, bool may_sync, bool first) {
    TimeTicker();
    if (may_sync && isCurrentThread()) {
        task();
        return nullptr;
    }

    auto ret = std::make_shared<Task>(std::move(task));
    {
        lock_guard<mutex> lck(_mtx_task);
        if (first) {
            _list_task.emplace_front(ret);
        } else {
            _list_task.emplace_back(ret);
        }
    }
    //写数据到管道,唤醒主线程
    _pipe.write("", 1);
    return ret;
}

bool EventPoller::isCurrentThread() {
    return !_loop_thread || _loop_thread->get_id() == this_thread::get_id();
}
// 执行处理异步队列中的任务，队列中的任务可以通过外部指针cancel
inline void EventPoller::onPipeEvent() {
    char buf[1024];
    int err = 0;
    while (true) {
        if ((err = _pipe.read(buf, sizeof(buf))) > 0) {
            // 读到管道数据,继续读,直到读空为止
            continue;
        }
        if (err == 0 || get_uv_error(true) != UV_EAGAIN) {
            // 收到eof或非EAGAIN(无更多数据)错误,说明管道无效了,重新打开管道
            ErrorL << "Invalid pipe fd of event poller, reopen it";
            delEvent(_pipe.readFD());
            _pipe.reOpen();
            addEventPipe();
        }
        break;
    }

    decltype(_list_task) _list_swap;
    {
        lock_guard<mutex> lck(_mtx_task);
        _list_swap.swap(_list_task);
    }

    _list_swap.for_each([&](const Task::Ptr &task) {
        try {
            (*task)();
        } catch (ExitException &) {
            _exit_flag = true;
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when do async task: " << ex.what();
        }
    });
}

static mutex s_all_poller_mtx;
static map<thread::id, weak_ptr<EventPoller> > s_all_poller;

BufferRaw::Ptr EventPoller::getSharedBuffer() {
    auto ret = _shared_buffer.lock();
    if (!ret) {
        //预留一个字节存放\0结尾符
        ret = BufferRaw::create();
        ret->setCapacity(1 + SOCKET_DEFAULT_BUF_SIZE);
        _shared_buffer = ret;
    }
    return ret;
}

thread::id EventPoller::getThreadId() const {
    return _loop_thread ? _loop_thread->get_id() : thread::id();
}

const std::string& EventPoller::getThreadName() const {
    return _name;
}

//static
// 获取当前线程的poller
EventPoller::Ptr EventPoller::getCurrentPoller(){
    lock_guard<mutex> lck(s_all_poller_mtx);
    auto it = s_all_poller.find(this_thread::get_id());
    if (it == s_all_poller.end()) {
        return nullptr;
    }
    return it->second.lock();
}

// ref_self注册为全局可见的poller线程
void EventPoller::runLoop(bool blocked,bool ref_self) {
    if (blocked) {
        if (ref_self) {
            lock_guard<mutex> lck(s_all_poller_mtx);
            s_all_poller[this_thread::get_id()] = shared_from_this();
        }
        _sem_run_started.post();
        _exit_flag = false;
        uint64_t minDelay;
#if defined(HAS_EPOLL)
        struct epoll_event events[EPOLL_SIZE];
        while (!_exit_flag) {
            minDelay = getMinDelay();
            startSleep();//用于统计当前线程负载情况
            int ret = epoll_wait(_epoll_fd, events, EPOLL_SIZE, minDelay ? minDelay : -1);
            sleepWakeUp();//用于统计当前线程负载情况
            if (ret <= 0) {
                //超时或被打断
                continue;
            }
            // delevent
            _event_cache_expired_map.clear();

            for (int i = 0; i < ret; ++i) {
                struct epoll_event &ev = events[i];
                int fd = ev.data.fd;
                if (_event_cache_expired_map.find(fd) != _event_cache_expired_map.end()) {
                    //event cache refresh
                    continue;
                }

                auto it = _event_map.find(fd);
                if (it == _event_map.end()) {
                    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    continue;
                }
                auto cb = it->second;
                try {
                    (*cb)(toPoller(ev.events));
                } catch (std::exception &ex) {
                    ErrorL << "Exception occurred when do event task: " << ex.what();
                }
            }
        }
#else
        int ret, max_fd;
        FdSet set_read, set_write, set_err;
        List<Poll_Record::Ptr> callback_list;
        struct timeval tv;
        while (!_exit_flag) {
            //定时器事件中可能操作_event_map
            minDelay = getMinDelay();
            tv.tv_sec = (decltype(tv.tv_sec)) (minDelay / 1000);
            tv.tv_usec = 1000 * (minDelay % 1000);

            set_read.fdZero();
            set_write.fdZero();
            set_err.fdZero();
            max_fd = 0;
            for (auto &pr : _event_map) {
                if (pr.first > max_fd) {
                    max_fd = pr.first;
                }
                if (pr.second->event & Event_Read) {
                    set_read.fdSet(pr.first);//监听管道可读事件
                }
                if (pr.second->event & Event_Write) {
                    set_write.fdSet(pr.first);//监听管道可写事件
                }
                if (pr.second->event & Event_Error) {
                    set_err.fdSet(pr.first);//监听管道错误事件
                }
            }

            startSleep();//用于统计当前线程负载情况
            ret = turbo_select(max_fd + 1, &set_read, &set_write, &set_err, minDelay ? &tv : nullptr);
            sleepWakeUp();//用于统计当前线程负载情况

            if (ret <= 0) {
                //超时或被打断
                continue;
            }

            _event_cache_expired_map.clear();

            //收集select事件类型
            for (auto &pr : _event_map) {
                int event = 0;
                if (set_read.isSet(pr.first)) {
                    event |= Event_Read;
                }
                if (set_write.isSet(pr.first)) {
                    event |= Event_Write;
                }
                if (set_err.isSet(pr.first)) {
                    event |= Event_Error;
                }
                if (event != 0) {
                    pr.second->attach = event;
                    callback_list.emplace_back(pr.second);
                }
            }

            callback_list.for_each([this](Poll_Record::Ptr &record) {
                if (this->_event_cache_expired_map.find(record->fd) != this->_event_cache_expired_map.end()) {
                    //event cache refresh
                    return;
                }

                try {
                    record->call_back(record->attach);
                } catch (std::exception &ex) {
                    ErrorL << "Exception occurred when do event task: " << ex.what();
                }
            });
            callback_list.clear();
        }
#endif //HAS_EPOLL
    } else {
        _loop_thread = new thread(&EventPoller::runLoop, this, true, ref_self);
        _sem_run_started.wait();
    }
}

uint64_t EventPoller::flushDelayTask(uint64_t now_time) {
    decltype(_delay_task_map) task_copy;
    task_copy.swap(_delay_task_map);

    for (auto it = task_copy.begin(); it != task_copy.end() && it->first <= now_time; it = task_copy.erase(it)) {
        //已到期的任务
        try {
            auto next_delay = (*(it->second))();
            if (next_delay) {
                //可重复任务,更新时间截止线
                _delay_task_map.emplace(next_delay + now_time, std::move(it->second));
            }
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when do delay task: " << ex.what();
        }
    }

    task_copy.insert(_delay_task_map.begin(), _delay_task_map.end());
    task_copy.swap(_delay_task_map);

    auto it = _delay_task_map.begin();
    if (it == _delay_task_map.end()) {
        //没有剩余的定时器了
        return 0;
    }
    //最近一个定时器的执行延时
    return it->first - now_time;
}

uint64_t EventPoller::getMinDelay() {
    auto it = _delay_task_map.begin();
    if (it == _delay_task_map.end()) {
        //没有剩余的定时器了
        return 0;
    }
    auto now = getCurrentMillisecond();
    if (it->first > now) {
        //所有任务尚未到期
        return it->first - now;
    }
    //执行已到期的任务并刷新休眠延时
    return flushDelayTask(now);
}

EventPoller::DelayTask::Ptr EventPoller::doDelayTask(uint64_t delay_ms, function<uint64_t()> task) {
    DelayTask::Ptr ret = std::make_shared<DelayTask>(std::move(task));
    auto time_line = getCurrentMillisecond() + delay_ms;
    async_first([time_line, ret, this]() {
        //异步执行的目的是刷新select或epoll的休眠时间
        _delay_task_map.emplace(time_line, ret);
    });
    return ret;
}


///////////////////////////////////////////////

static size_t s_pool_size = 0;
static bool s_enable_cpu_affinity = true;

INSTANCE_IMP(EventPollerPool)

EventPoller::Ptr EventPollerPool::getFirstPoller() {
    return static_pointer_cast<EventPoller>(_threads.front());
}

EventPoller::Ptr EventPollerPool::getPoller(bool prefer_current_thread) {
    auto poller = EventPoller::getCurrentPoller();
    if (prefer_current_thread && _prefer_current_thread && poller) {
        return poller;
    }
    return static_pointer_cast<EventPoller>(getExecutor());
}

void EventPollerPool::preferCurrentThread(bool flag) {
    _prefer_current_thread = flag;
}

const std::string EventPollerPool::kOnStarted = "kBroadcastEventPollerPoolStarted";

EventPollerPool::EventPollerPool() {
    auto size = addPoller("event poller", s_pool_size, static_cast<int>(ThreadPool::Priority::PRIORITY_HIGHEST), true, s_enable_cpu_affinity);
    NOTICE_EMIT(EventPollerPoolOnStartedArgs, kOnStarted, *this, size);
    InfoL << "EventPoller created size: " << size;
}

void EventPollerPool::setPoolSize(size_t size) {
    s_pool_size = size;
}

void EventPollerPool::enableCpuAffinity(bool enable) {
    s_enable_cpu_affinity = enable;
}

}  // namespace turbo

