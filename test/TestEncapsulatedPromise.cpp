#include <iostream>
#include <tev-cpp/Tev.h>
#include "../include/Promise.h"
#include "TestUtility.h"

template <typename T>
class Request
{
public:
    class cancelled_exception : public std::exception
    {
    };

    static Request<T> MakeRequestAsync(Tev& tev, int ms, T value)
    {
        JS::Promise<T> promise{};
        /** Use a timeout to simulate an async request */
        Tev::TimeoutHandle handle = tev.SetTimeout([=]() {
            promise.Resolve(value);
        }, ms);
        return Request<T>(promise, tev, handle);
    }

    bool await_ready() const
    {
        return _promise.await_ready();
    }
    void await_suspend(std::coroutine_handle<> handle)
    {
        _promise.await_suspend(handle);
    }
    T await_resume()
    {
        return std::move(_promise.await_resume());
    }

    void Cancel() const
    {
        /** The actual work needs to be cancelled as well */
        _tev.ClearTimeout(_handle);
        if (!_promise.await_ready())
        {
            _promise.Reject(std::make_exception_ptr(cancelled_exception{}));
        }
    }
private:
    Request(const JS::Promise<T>& promise, Tev& tev, const Tev::TimeoutHandle& handle)
        : _promise(promise), _tev(tev), _handle(handle)
    {
    }
    JS::Promise<T> _promise;
    Tev& _tev;
    Tev::TimeoutHandle _handle;
};


static Tev tev{};

JS::Promise<void> TestNormalRequestAsync()
{
    auto result = co_await Request<int>::MakeRequestAsync(tev, 1000, 42);
    assert(result == 42, "wrong result");
}

JS::Promise<void> TestCancelledRequestAsync()
{
    auto request = Request<int>::MakeRequestAsync(tev, 1000, 100);
    /** Cancel the request before it completes */
    tev.SetTimeout([=](){
        request.Cancel();
    }, 500);
    try
    {
        co_await request;
    }
    catch(const Request<int>::cancelled_exception& e)
    {
        co_return;
    }
    assert(false, "Request should have been cancelled");
}

JS::Promise<void> TestAsync()
{
    RunAsyncTest(TestNormalRequestAsync);
    RunAsyncTest(TestCancelledRequestAsync);
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;

    TestAsync();

    tev.MainLoop();

    return 0;
}
