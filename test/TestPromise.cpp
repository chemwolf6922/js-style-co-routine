#include <iostream>
#include <memory>
#include <tev-cpp/Tev.h>
#include "../include/Promise.h"
#include "TestUtility.h"

static Tev tev{};

static JS::Promise<void> DelayAsync(int ms)
{
    JS::Promise<void> promise{};
    tev.SetTimeout([=]() {
        promise.Resolve();
    }, ms);
    return promise;
}

static JS::Promise<int> ResolveImmediatelyAsync(int value)
{
    JS::Promise<int> promise{};
    promise.Resolve(value);
    return promise;
}

static JS::Promise<int> ResolveAfterDelayAsync(int ms, int value)
{
    JS::Promise<int> promise{};
    tev.SetTimeout([=]() {
        promise.Resolve(value);
    }, ms);
    return promise;
}

static JS::Promise<std::unique_ptr<int>> ResolveNonCopyableImmediatelyAsync(int value)
{
    JS::Promise<std::unique_ptr<int>> promise{};
    promise.Resolve(std::make_unique<int>(value));
    return promise;
}

static JS::Promise<std::unique_ptr<int>> ResolveNonCopyableAfterDelayAsync(int ms, int value)
{
    JS::Promise<std::unique_ptr<int>> promise{};
    tev.SetTimeout([=]() {
        promise.Resolve(std::make_unique<int>(value));
    }, ms);
    return promise;
}

JS::Promise<void> TestResolveAsync()
{
    int value = co_await ResolveImmediatelyAsync(42);
    assert(value == 42, "ResolveImmediatelyAsync");

    value = co_await ResolveAfterDelayAsync(100, 42);
    assert(value == 42, "ResolveAfterDelayAsync");

    auto ptr = co_await ResolveNonCopyableImmediatelyAsync(42);
    assert(ptr && *ptr == 42, "ResolveNonCopyableImmediatelyAsync");

    auto ptr2 = co_await ResolveNonCopyableAfterDelayAsync(100, 42);
    assert(ptr2 && *ptr2 == 42, "ResolveNonCopyableAfterDelayAsync");
}

JS::Promise<void> TestThenImmediateAsync()
{
    /** Help tracking the test */
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = ResolveImmediatelyAsync(42);
        promise.Then([=](auto value){
            assert(value == 42, "wrong result");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

JS::Promise<void> TestThenAsync()
{
    /** Help tracking the test */
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = ResolveAfterDelayAsync(100, 42);
        promise.Then([=](auto value){
            assert(value == 42, "wrong result");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

static JS::Promise<int> RejectImmediatelyAsync(const std::string& reason)
{
    JS::Promise<int> promise{};
    promise.Reject(reason);
    return promise;
}

static JS::Promise<int> RejectAfterDelayAsync(int ms, const std::string& reason)
{
    JS::Promise<int> promise{};
    tev.SetTimeout([=]() {
        promise.Reject(reason);
    }, ms);
    return promise;
}

JS::Promise<void> TestRejectAsync()
{
    try
    {
        co_await RejectImmediatelyAsync("Immediate rejection");
        assert(false, "RejectImmediatelyAsync should have thrown");
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Immediate rejection", "RejectImmediatelyAsync wrong reason");
    }
    try
    {
        co_await RejectAfterDelayAsync(100, "Delayed rejection");
        assert(false, "RejectAfterDelayAsync should have thrown");
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Delayed rejection", "RejectAfterDelayAsync wrong reason");
    }
}

JS::Promise<void> TestCatchImmediatelyAsync()
{
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = RejectImmediatelyAsync("Immediate rejection");
        promise.Catch([=](const std::exception &e){
            assert(std::string(e.what()) == "Immediate rejection", "wrong reason");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

JS::Promise<void> TestCatchAsync()
{
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = RejectAfterDelayAsync(100, "Delayed rejection");
        promise.Catch([=](const std::exception &e){
            assert(std::string(e.what()) == "Delayed rejection", "wrong reason");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

static JS::Promise<int> CoRoutineReturnImmediatelyAsync(int value)
{
    co_return value;
}

static JS::Promise<int> CoRoutineReturnAfterDelayAsync(int ms, int value)
{
    co_await DelayAsync(ms);
    co_return value;
}

static JS::Promise<std::unique_ptr<int>> CoRoutineReturnNonCopyableImmediatelyAsync(int value)
{
    co_return std::make_unique<int>(value);
}

static JS::Promise<std::unique_ptr<int>> CoRoutineReturnNonCopyableAfterDelayAsync(int ms, int value)
{
    co_await DelayAsync(ms);
    co_return std::make_unique<int>(value);
}

JS::Promise<void> TestCoRoutineReturnAsync()
{
    int value = co_await CoRoutineReturnImmediatelyAsync(42);
    assert(value == 42, "CoRoutineReturnImmediatelyAsync");

    value = co_await CoRoutineReturnAfterDelayAsync(100, 42);
    assert(value == 42, "CoRoutineReturnAfterDelayAsync");

    auto ptr = co_await CoRoutineReturnNonCopyableImmediatelyAsync(42);
    assert(ptr && *ptr == 42, "CoRoutineReturnNonCopyableImmediatelyAsync");

    auto ptr2 = co_await CoRoutineReturnNonCopyableAfterDelayAsync(100, 42);
    assert(ptr2 && *ptr2 == 42, "CoRoutineReturnNonCopyableAfterDelayAsync");
}

JS::Promise<void> TestCoRoutineThenImmediateAsync()
{
    /** Help tracking the test */
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = CoRoutineReturnImmediatelyAsync(42);
        promise.Then([=](auto value){
            assert(value == 42, "wrong result");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

JS::Promise<void> TestCoRoutineThenAsync()
{
    /** Help tracking the test */
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = CoRoutineReturnAfterDelayAsync(100, 42);
        promise.Then([=](auto value){
            assert(value == 42, "wrong result");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

static JS::Promise<int> CoRoutineThrowImmediatelyAsync(const std::string reason)
{
    throw std::runtime_error(reason);
}

static JS::Promise<int> CoRoutineThrowAfterDelayAsync(int ms, const std::string reason)
{
    co_await DelayAsync(ms);
    throw std::runtime_error(reason);
}

JS::Promise<void> TestCoRoutineThrowAsync()
{
    try
    {
        co_await CoRoutineThrowImmediatelyAsync("Immediate throw");
        assert(false, "CoRoutineThrowImmediatelyAsync should have thrown");
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Immediate throw", "CoRoutineThrowImmediatelyAsync wrong reason");
    }
    try
    {
        co_await CoRoutineThrowAfterDelayAsync(100, "Delayed throw");
        assert(false, "CoRoutineThrowAfterDelayAsync should have thrown");
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Delayed throw", "CoRoutineThrowAfterDelayAsync wrong reason");
    }
}

JS::Promise<void> TestCoRoutineCatchAsync()
{
    JS::Promise<void> testPromise{};
    {
        /** Fire the promise in a limited scope */
        auto promise = CoRoutineThrowAfterDelayAsync(100, "Delayed throw");
        promise.Catch([=](const std::exception &e){
            assert(std::string(e.what()) == "Delayed throw", "wrong reason");
            testPromise.Resolve();
        });
    }
    co_await testPromise;
}

JS::Promise<void> TestPromiseAllResolveImmediatelyAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveImmediatelyAsync(2)));
    promises.push_back(std::move(CoRoutineReturnImmediatelyAsync(4)));
    auto results = co_await JS::Promise<int>::All(promises);
    assert(results.size() == 2, "size mismatch");
    assert(results[0] == 2, "first value mismatch");
    assert(results[1] == 4, "second value mismatch");
}

JS::Promise<void> TestPromiseAllResolveAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(ResolveImmediatelyAsync(2)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 3)));
    promises.push_back(std::move(CoRoutineReturnImmediatelyAsync(4)));
    auto results = co_await JS::Promise<int>::All(promises);
    assert(results.size() == 4, "size mismatch");
    assert(results[0] == 1, "first value mismatch");
    assert(results[1] == 2, "second value mismatch");
    assert(results[2] == 3, "third value mismatch");
    assert(results[3] == 4, "fourth value mismatch");
}

JS::Promise<void> TestPromiseAllRejectImmediatelyAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(ResolveImmediatelyAsync(2)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 3)));
    promises.push_back(std::move(CoRoutineReturnImmediatelyAsync(4)));
    promises.push_back(std::move(RejectImmediatelyAsync("Error in promise 5")));
    try
    {
        co_await JS::Promise<int>::All(promises);
    }
    catch(const std::exception& e)
    {
        co_return;
    }
    assert(false, "should have thrown");
}

JS::Promise<void> TestPromiseAllRejectAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(ResolveImmediatelyAsync(2)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 3)));
    promises.push_back(std::move(CoRoutineReturnImmediatelyAsync(4)));
    promises.push_back(std::move(RejectAfterDelayAsync(200, "Error in promise 5")));
    try
    {
        co_await JS::Promise<int>::All(promises);
    }
    catch(const std::exception& e)
    {
        co_return;
    }
    assert(false, "should have thrown");
}

JS::Promise<void> TestPromiseAnyResolveImmediatelyAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(ResolveImmediatelyAsync(2)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 3)));
    promises.push_back(std::move(CoRoutineReturnImmediatelyAsync(4)));
    auto result = co_await JS::Promise<int>::Any(promises);
    assert(result == 2, "wrong result");
}

JS::Promise<void> TestPromiseAnyResolveAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 3)));
    auto result = co_await JS::Promise<int>::Any(promises);
    assert(result == 1, "wrong result");
}

JS::Promise<void> TestPromiseAnyRejectImmediatelyAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    try
    {
        promises.push_back(std::move(RejectImmediatelyAsync("Error in promise 1")));
        promises.push_back(std::move(CoRoutineThrowImmediatelyAsync("Error in promise 2")));
        co_await JS::Promise<int>::Any(promises);
    }
    catch(const std::exception& e)
    {
        co_return;
    }
    assert(false, "should have thrown");
}

JS::Promise<void> TestPromiseAnyRejectAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(RejectAfterDelayAsync(100, "Error in promise 1")));
    promises.push_back(std::move(CoRoutineThrowAfterDelayAsync(300, "Error in promise 2")));
    try
    {
        co_await JS::Promise<int>::Any(promises);
    }
    catch(const std::exception& e)
    {
        co_return;
    }
    assert(false, "should have thrown");
}

JS::Promise<void> TestPromiseRaceResolveImmediatelyAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(ResolveImmediatelyAsync(2)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 3)));
    promises.push_back(std::move(CoRoutineReturnImmediatelyAsync(4)));
    promises.push_back(std::move(RejectAfterDelayAsync(200, "Error in promise 5")));
    promises.push_back(std::move(CoRoutineThrowAfterDelayAsync(400, "Error in promise 6")));
    auto result = co_await JS::Promise<int>::Race(promises);
    assert(result == 2 || result == 4, "wrong result");
}

JS::Promise<void> TestPromiseRaceResolveAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 2)));
    promises.push_back(std::move(RejectAfterDelayAsync(200, "Error in promise 3")));
    promises.push_back(std::move(CoRoutineThrowAfterDelayAsync(400, "Error in promise 4")));
    auto result = co_await JS::Promise<int>::Race(promises);
    assert(result == 1, "wrong result");
}

JS::Promise<void> TestPromiseRaceRejectImmediatelyAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    try
    {
        promises.push_back(std::move(ResolveAfterDelayAsync(100, 1)));
        promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(300, 2)));
        promises.push_back(std::move(RejectImmediatelyAsync("Error in promise 3")));
        promises.push_back(std::move(CoRoutineThrowImmediatelyAsync("Error in promise 4")));
        promises.push_back(std::move(RejectAfterDelayAsync(200, "Error in promise 5")));
        promises.push_back(std::move(CoRoutineThrowAfterDelayAsync(400, "Error in promise 6")));
        co_await JS::Promise<int>::Race(promises);
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Error in promise 3" || std::string(e.what()) == "Error in promise 4",
            "wrong promise rejection reason");
        co_return;
    }
    assert(false, "should have thrown");
}

JS::Promise<void> TestPromiseRaceRejectAsync()
{
    auto promises = std::vector<JS::Promise<int>>{};
    promises.push_back(std::move(ResolveAfterDelayAsync(200, 1)));
    promises.push_back(std::move(CoRoutineReturnAfterDelayAsync(400, 2)));
    promises.push_back(std::move(RejectAfterDelayAsync(100, "Error in promise 3")));
    promises.push_back(std::move(CoRoutineThrowAfterDelayAsync(300, "Error in promise 4")));
    try
    {
        co_await JS::Promise<int>::Race(promises);
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Error in promise 3", "wrong promise rejection reason");
        co_return;
    }
    assert(false, "should have thrown");
}

JS::Promise<void> TestAsync()
{
    RunAsyncTest(TestResolveAsync);
    RunAsyncTest(TestThenImmediateAsync);
    RunAsyncTest(TestThenAsync);
    RunAsyncTest(TestRejectAsync);
    RunAsyncTest(TestCatchImmediatelyAsync);
    RunAsyncTest(TestCatchAsync);
    RunAsyncTest(TestCoRoutineReturnAsync);
    RunAsyncTest(TestCoRoutineThenImmediateAsync);
    RunAsyncTest(TestCoRoutineThenAsync);
    RunAsyncTest(TestCoRoutineThrowAsync);
    /** Catch does not work on a immediately thrown coroutine */
    RunAsyncTest(TestCoRoutineCatchAsync);
    RunAsyncTest(TestPromiseAllResolveImmediatelyAsync);
    RunAsyncTest(TestPromiseAllResolveAsync);
    RunAsyncTest(TestPromiseAllRejectImmediatelyAsync);
    RunAsyncTest(TestPromiseAllRejectAsync);
    RunAsyncTest(TestPromiseAnyResolveImmediatelyAsync);
    RunAsyncTest(TestPromiseAnyResolveAsync);
    RunAsyncTest(TestPromiseAnyRejectImmediatelyAsync);
    RunAsyncTest(TestPromiseAnyRejectAsync);
    RunAsyncTest(TestPromiseRaceResolveImmediatelyAsync);
    RunAsyncTest(TestPromiseRaceResolveAsync);
    RunAsyncTest(TestPromiseRaceRejectImmediatelyAsync);
    RunAsyncTest(TestPromiseRaceRejectAsync);
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;

    TestAsync();

    tev.MainLoop();

    return 0;
}
