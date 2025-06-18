#include <vector>
#include <memory>
#include <iostream>
#include <tev-cpp/Tev.h>
#include "../include/AsyncGenerator.h"
#include "TestUtility.h"

static Tev tev{};

JS::Promise<void> DelayAsync(int ms)
{
    JS::Promise<void> promise{};
    tev.SetTimeout([=]() {
        promise.Resolve();
    }, ms);
    return promise;
}

JS::AsyncGenerator<int> GenNumbersAsync(int start, int end)
{
    for (int i = start; i <= end; i++)
    {
        co_yield i;
        co_await DelayAsync(100);
    }
}

JS::AsyncGenerator<int, bool> GenNumberWithReturnAsync(int start, int end, bool returnValue)
{
    for (int i = start; i <= end; i++)
    {
        co_yield i;
        co_await DelayAsync(100);
    }
    co_return returnValue;
}

JS::AsyncGenerator<std::unique_ptr<int>> GenNumbersNonCopyableAsync(int start, int end)
{
    for (int i = start; i <= end; i++)
    {
        co_yield std::make_unique<int>(i);
        co_await DelayAsync(100);
    }
}

JS::AsyncGenerator<std::unique_ptr<int>, std::unique_ptr<bool>> GenNumbersNonCopyableWithReturnAsync(int start, int end, bool returnValue)
{
    for (int i = start; i <= end; i++)
    {
        co_yield std::make_unique<int>(i);
        co_await DelayAsync(100);
    }
    co_return std::make_unique<bool>(returnValue);
}

JS::AsyncGenerator<int> GenExceptionAsync(const std::string reason)
{
    co_await DelayAsync(100);
    throw std::runtime_error(reason);
}

JS::Promise<void> TestGenNumbersAsync()
{
    size_t start = 1;
    size_t end = 5;
    std::vector<int> result{};
    auto gen = GenNumbersAsync(static_cast<int>(start), static_cast<int>(end));
    while (true)
    {
        auto next = co_await gen.NextAsync();
        if (!next.has_value())
        {
            break;
        }
        result.push_back(next.value());
    }
    assert(result.size() == (end - start + 1), "Generator did not yield the expected number of values");
    int expectedValue = start;
    for (const auto &value : result)
    {
        assert(value == expectedValue, "Generator yielded unexpected value");
        expectedValue++;
    }
}

JS::Promise<void> TestReturnValueAsync()
{
    size_t start = 1;
    size_t end = 5;
    std::vector<int> result{};
    auto gen = GenNumberWithReturnAsync(static_cast<int>(start), static_cast<int>(end), true);
    while (true)
    {
        auto next = co_await gen.NextAsync();
        if (!next.has_value())
        {
            break;
        }
        result.push_back(next.value());
    }
    auto returnValue = gen.GetReturnValue();
    assert(returnValue == true, "Generator did not return the expected value");
    assert(result.size() == (end - start + 1), "Generator did not yield the expected number of values");
    int expectedValue = start;
    for (const auto &value : result)
    {
        assert(value == expectedValue, "Generator yielded unexpected value");
        expectedValue++;
    }
}

JS::Promise<void> TestGenNumbersNonCopyableAsync()
{
    size_t start = 1;
    size_t end = 5;
    std::vector<int> result{};
    auto gen = GenNumbersNonCopyableAsync(static_cast<int>(start), static_cast<int>(end));
    while (true)
    {
        auto next = co_await gen.NextAsync();
        if (!next.has_value())
        {
            break;
        }
        result.push_back(*next.value());
    }
    assert(result.size() == (end - start + 1), "Generator did not yield the expected number of values");
    int expectedValue = start;
    for (const auto &value : result)
    {
        assert(value == expectedValue, "Generator yielded unexpected value");
        expectedValue++;
    }
}

JS::Promise<void> TestGenNumbersNonCopyableWithReturnAsync()
{
    size_t start = 1;
    size_t end = 5;
    std::vector<int> result{};
    auto gen = GenNumbersNonCopyableWithReturnAsync(static_cast<int>(start), static_cast<int>(end), true);
    while (true)
    {
        auto next = co_await gen.NextAsync();
        if (!next.has_value())
        {
            break;
        }
        result.push_back(*next.value());
    }
    auto returnValue = gen.GetReturnValue();
    assert(*returnValue == true, "Generator did not return the expected value");
    assert(result.size() == (end - start + 1), "Generator did not yield the expected number of values");
    int expectedValue = start;
    for (const auto &value : result)
    {
        assert(value == expectedValue, "Generator yielded unexpected value");
        expectedValue++;
    }
}

JS::Promise<void> TestGenExceptionAsync()
{
    auto gen = GenExceptionAsync("Test exception");
    try
    {
        co_await gen.NextAsync();
        assert(false, "Should have thrown an exception");
    }
    catch(const std::exception& e)
    {
        assert(std::string(e.what()) == "Test exception", "Generator did not throw the expected exception");
    }
}

JS::Promise<void> TestAsync()
{
    RunAsyncTest(TestGenNumbersAsync);
    RunAsyncTest(TestReturnValueAsync);
    RunAsyncTest(TestGenNumbersNonCopyableAsync);
    RunAsyncTest(TestGenNumbersNonCopyableWithReturnAsync);
    RunAsyncTest(TestGenExceptionAsync);
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;

    TestAsync();

    tev.MainLoop();

    return 0;
}
