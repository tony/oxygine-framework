#include "ThreadDispatcher.h"
#include "log.h"
#include "pthread.h"
#include "Mutex.h"

namespace oxygine
{
    static size_t threadID()
    {
        pthread_t pt = pthread_self();
        return ((size_t*)(&pt))[0];
    }

#if 0
#define  LOGDN(format, ...)  log::messageln("ThreadMessages(%lu)::" format, threadID(), __VA_ARGS__)
#else
#define  LOGDN(...)  ((void)0)
#endif


    void addtime(timespec& ts, int ms)
    {
        ts.tv_nsec += ms * 1000000;
        while (ts.tv_nsec >= 1000000000)
        {
            ts.tv_nsec -= 1000000000;
            ++ts.tv_sec;
        }
    }


    void mywait(pthread_cond_t* cond, pthread_mutex_t* mutex)
    {
#ifdef __S3E__
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        addtime(ts, 300);
        pthread_cond_timedwait(cond, mutex, &ts);
#elif __ANDROID__
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        addtime(ts, 500);
        pthread_cond_timedwait(cond, mutex, &ts);
#else
        pthread_cond_wait(cond, mutex);
#endif
    }


    MutexPthreadLock::MutexPthreadLock(pthread_mutex_t& m, bool lock) : _mutex(m), _locked(lock)
    {
        if (_locked)
            pthread_mutex_lock(&_mutex);
    }

    MutexPthreadLock::~MutexPthreadLock()
    {
        pthread_mutex_unlock(&_mutex);
    }

    ThreadDispatcher::ThreadDispatcher(): _id(0), _result(0)
    {
        pthread_cond_init(&_cond, 0);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&_mutex, &attr);

        _events.reserve(10);
    }

    ThreadDispatcher::~ThreadDispatcher()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }

    void ThreadDispatcher::_waitMessage()
    {
        _replyLast(0);

        while (_events.empty())
            pthread_cond_wait(&_cond, &_mutex);
    }

    void ThreadDispatcher::wait()
    {
        MutexPthreadLock lock(_mutex);
        _waitMessage();
    }

    void ThreadDispatcher::get(message& ev)
    {
        MutexPthreadLock lock(_mutex);
        LOGDN("get");

        _waitMessage();
        _popMessage(ev);
    }

    bool ThreadDispatcher::empty()
    {
        MutexPthreadLock lock(_mutex);
        bool v = _events.empty();
        return v;
    }

    size_t ThreadDispatcher::size()
    {
        MutexPthreadLock lock(_mutex);
        size_t v = _events.size();
        return v;
    }

    void ThreadDispatcher::clear()
    {
        MutexPthreadLock lock(_mutex);
        _events.clear();
    }

    void ThreadDispatcher::_popMessage(message& res)
    {
        _last = _events.front();
        _events.erase(_events.begin());

        if (_last.cb)
        {
            LOGDN("running callback for id=%d", _last._id);
            _last.cb(_last);
            _last.cb = 0;
        }

#ifndef __S3E__
        if (_last.cbFunction)
        {
            LOGDN("running callback function for id=%d", _last._id);
            _last.cbFunction();
            _last.cbFunction = std::function< void() >();
        }
#endif

        LOGDN("_gotMessage id=%d, msgid=%d", _last._id, _last.msgid);

        if (!_last.msgid)
        {
            _replyLast(0);
        }

        res = _last;
    }

    bool ThreadDispatcher::peek(peekMessage& ev, bool del)
    {
        if (!ev.num)
            return false;


        MutexPthreadLock lock(_mutex);
        if (ev.num == -1)
            ev.num = (int) _events.size();

        LOGDN("peeking message");

        _replyLast(0);

        if (!_events.empty() && ev.num > 0)
        {
            if (del)
                _popMessage(ev);
            ev.num--;

            return true;
        }

        return false;
    }

    void ThreadDispatcher::_replyLast(void* val)
    {
        _replyingTo = _last._id;
        _result = val;

        while (_last.need_reply)
        {
            LOGDN("replying to id=%d", _last._id);

            //pthread_cond_signal(&_cond);
            pthread_cond_broadcast(&_cond);
            mywait(&_cond, &_mutex);
        }
    }



    void ThreadDispatcher::_waitReply(int id)
    {
        do
        {
            LOGDN("ThreadMessages::waiting reply... _replyingTo=%d  myid=%d", _replyingTo, id);
            pthread_cond_signal(&_cond);
            mywait(&_cond, &_mutex);
        }
        while (_replyingTo != id);

        _last.need_reply = false;
        pthread_cond_signal(&_cond);
    }

    void ThreadDispatcher::reply(void* val)
    {
        MutexPthreadLock lock(_mutex);
        _replyLast(val);
    }


    void* ThreadDispatcher::send(int msgid, void* arg1, void* arg2)
    {
        OX_ASSERT(msgid);

        message ev;
        ev.msgid = msgid;
        ev.arg1 = arg1;
        ev.arg2 = arg2;

        MutexPthreadLock lock(_mutex);
        _pushMessageWaitReply(ev);

        return _result;
    }

    void ThreadDispatcher::sendCallback(void* arg1, void* arg2, callback cb, void* cbData)
    {
        message ev;
        ev.msgid = 0;
        ev.arg1 = arg1;
        ev.arg2 = arg2;
        ev.cb = cb;
        ev.cbData = cbData;

        MutexPthreadLock lock(_mutex);
        _pushMessageWaitReply(ev);
    }

    void ThreadDispatcher::_pushMessageWaitReply(message& msg)
    {
        msg._id = ++_id;
        msg.need_reply = true;
        LOGDN("_pushMessageWaitReply id=%d msgid=%d", msg._id, msg.msgid);


        _events.push_back(msg);
        _waitReply(msg._id);
        LOGDN("waiting reply  %d - done", msg._id);
    }

    void ThreadDispatcher::_pushMessage(message& msg)
    {
        msg._id = ++_id;
        _events.push_back(msg);
        pthread_cond_signal(&_cond);
    }

    void ThreadDispatcher::post(int msgid, void* arg1, void* arg2)
    {
        message ev;
        ev.msgid = msgid;
        ev.arg1 = arg1;
        ev.arg2 = arg2;

        MutexPthreadLock lock(_mutex);
        _pushMessage(ev);
    }

    void ThreadDispatcher::postCallback(int msgid, void* arg1, void* arg2, callback cb, void* cbData)
    {
        message ev;
        ev.msgid = msgid;
        ev.arg1 = arg1;
        ev.arg2 = arg2;
        ev.cb = cb;
        ev.cbData = cbData;

        MutexPthreadLock lock(_mutex);
        _pushMessage(ev);
    }

    void ThreadDispatcher::removeCallback(int msgid, callback cb, void* cbData)
    {
        MutexPthreadLock lock(_mutex);
        for (messages::iterator i = _events.begin(); i != _events.end(); ++i)
        {
            message& m = *i;
            if (m.cb == cb && m.cbData == cbData && m.msgid == msgid)
            {
                _events.erase(i);
                break;
            }
        }
    }

#ifndef __S3E__
    void ThreadDispatcher::postCallback(const std::function<void()>& f)
    {
        message ev;
        ev.msgid = 0;
        ev.arg1 = 0;
        ev.arg2 = 0;
        ev.cb = 0;
        ev.cbData = 0;
        ev.cbFunction = f;

        MutexPthreadLock lock(_mutex);
        _pushMessage(ev);
    }

    void ThreadDispatcher::sendCallback(const std::function<void()>& f)
    {
        message ev;
        ev.cbFunction = f;

        MutexPthreadLock lock(_mutex);
        _pushMessageWaitReply(ev);
    }
#endif
}