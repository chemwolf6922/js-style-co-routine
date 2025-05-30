#pragma once

#define assert(condition, message) \
    if (!(condition)) \
    { \
        std::cerr << __func__ << " failed: " << message << std::endl; \
        abort(); \
    }

#define RunAsyncTest(testFunc) \
    do \
    { \
        std::cout << "Running " << #testFunc << std::endl; \
        try \
        { \
            co_await testFunc(); \
        } \
        catch(const std::exception& e) \
        { \
            assert(false, std::string(#testFunc) + " failed with unhandled exception: " + e.what()); \
        } \
        std::cout << #testFunc << " completed successfully." << std::endl; \
    } while (0)
