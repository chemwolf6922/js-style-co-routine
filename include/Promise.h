#pragma once

#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

/**
 * Drawbacks compare to a real JavaScript Promise:
 * 1. This should not be awaited multiple times.
 * 2. Be careful with the value's lifetime. C++ is not safe.
 */

namespace JS
{
    template <typename T>
    struct Promise
    {
        struct State
        {
            std::optional<T> value;
            std::exception_ptr exception = nullptr;
            std::coroutine_handle<> callingHandle = nullptr;
            std::function<void(T)> thenCallback = nullptr;
            std::function<void(const std::exception &)> catchCallback = nullptr;
            void Resolve(T &&v)
            {
                value = std::move(v);
                if (callingHandle)
                {
                    callingHandle.resume();
                }
                else if (thenCallback)
                {
                    auto temp = std::move(value.value());
                    value.reset();
                    thenCallback(std::move(temp));
                }
            }
            void Resolve(const T &v)
            {
                value = v;
                if (callingHandle)
                {
                    callingHandle.resume();
                }
                else if (thenCallback)
                {
                    auto temp = std::move(value.value());
                    value.reset();
                    thenCallback(std::move(temp));
                }
            }
            void Reject(const std::exception_ptr &e)
            {
                exception = e;
                if (callingHandle)
                {
                    callingHandle.resume();
                }
                else if (catchCallback)
                {
                    try
                    {
                        std::rethrow_exception(exception);
                    }
                    catch (const std::exception &ex)
                    {
                        catchCallback(ex);
                    }
                }
            }
            void Reject(const std::string &reason)
            {
                Reject(std::make_exception_ptr(std::runtime_error(reason)));
            }
        };

        struct promise_type
        {
            std::shared_ptr<State> state = std::make_shared<State>();
            Promise<T> get_return_object()
            {
                return Promise{state};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_value(T &&v)
            {
                state->Resolve(std::move(v));
            }
            void return_value(const T &v)
            {
                state->Resolve(v);
            }
            void unhandled_exception()
            {
                state->Reject(std::current_exception());
            }
        };

        /**
         * @note DO NOT call this directly.
         *
         * This is part of the awaitable interface.
         * When awaited, this is called first.
         * If this returns true, the calling coroutine will not be suspended.
         * And await_resume will be called immediately.
         * If this returns false, the calling coroutine will be suspended.
         * And await_suspend will be called with the calling coroutine's handle.
         */
        bool await_ready() const
        {
            return _state->value.has_value() || _state->exception != nullptr;
        }

        /**
         * @note DO NOT call this directly.
         *
         * @param handle The calling coroutine's handle.
         */
        void await_suspend(std::coroutine_handle<> handle)
        {
            _state->callingHandle = handle;
        }

        /**
         * @note DO NOT call this directly.
         *
         * @return T result of the awaitable.
         */
        T await_resume()
        {
            if (_state->exception != nullptr)
            {
                std::rethrow_exception(_state->exception);
            }
            return std::move(std::exchange(_state->value, std::nullopt)).value();
        }

        /**
         * This is called when this is created as a coroutine.
         * Which should also be able to act as an awaitable.
         */
        Promise(std::shared_ptr<State> state)
            : _state(std::move(state))
        {
        }

        /**
         * This is called when this is created as an awaitable.
         * Which will not act as a coroutine.
         */
        Promise()
            : _state(std::make_shared<State>())
        {
        }

        /**
         * @brief Resolve the promise with a value. The value will be moved internally.
         *
         * @param v
         */
        void Resolve(T &&v) const
        {
            _state->Resolve(std::move(v));
        }

        /**
         * @brief Resolve the promise with a value. The value will be copied internally.
         *
         * @param v
         */
        void Resolve(const T &v) const
        {
            _state->Resolve(v);
        }

        void Reject(const std::exception_ptr &e) const
        {
            _state->Reject(e);
        }

        void Reject(const std::string &reason) const
        {
            _state->Reject(reason);
        }

        void Then(std::function<void(T)> callback)
        {
            /** This should only be called if this is not awaited */
            if (_state->callingHandle)
            {
                throw std::runtime_error("Promise is already awaited");
            }
            _state->thenCallback = callback;
            if (_state->value.has_value())
            {
                auto temp = std::move(_state->value.value());
                _state->value.reset();
                _state->thenCallback(std::move(temp));
            }
        }

        void Catch(std::function<void(const std::exception &)> callback)
        {
            if (_state->callingHandle)
            {
                throw std::runtime_error("Promise is already awaited");
            }
            _state->catchCallback = callback;
            if (_state->exception != nullptr)
            {
                try
                {
                    std::rethrow_exception(_state->exception);
                }
                catch (const std::exception &e)
                {
                    _state->catchCallback(e);
                }
            }
        }

        /**
         * @brief Wait for all promises to resolve or any to reject.
         * @param promises
         * @return Promise<std::vector<T>>
         */
        static Promise<std::vector<T>> All(std::vector<Promise<T>> &promises)
        {
            if (promises.empty())
            {
                /** In JS, this will resolve []. We choose to forbid this. */
                throw std::invalid_argument("Empty promises");
            }
            auto resultPromise = Promise<std::vector<T>>{};
            struct Result
            {
                std::vector<T> values{};
                size_t pending{};
                bool rejected{};
            };
            auto result = std::make_shared<Result>();
            result->values.resize(promises.size());
            result->pending = promises.size();
            result->rejected = false;
            for (size_t i = 0; i < promises.size(); ++i)
            {
                promises[i].Then([=](const T &value)
                                 {
                if (result->rejected)
                {
                    return;
                }
                result->values[i] = value;
                if (--(result->pending) == 0)
                {
                    resultPromise.Resolve(result->values);
                } });
                promises[i].Catch([=](const std::exception &e)
                                  {      
                if (result->rejected)
                {
                    return;
                }
                result->rejected = true;
                resultPromise.Reject(e.what()); });
            }
            return resultPromise;
        }

        static Promise<T> Any(std::vector<Promise<T>> &promises)
        {
            if (promises.empty())
            {
                /** In JS, this will reject []. We choose to forbid this. */
                throw std::invalid_argument("Empty promises");
            }
            auto resultPromise = Promise<T>{};
            struct Result
            {
                size_t pending{};
                bool resolved{};
            };
            auto result = std::make_shared<Result>();
            result->pending = promises.size();
            result->resolved = false;
            for (size_t i = 0; i < promises.size(); ++i)
            {
                promises[i].Then([=](const T &value)
                                 {
                if (result->resolved)
                {
                    return;
                }
                result->resolved = true;
                resultPromise.Resolve(value); });
                promises[i].Catch([=](const std::exception &)
                                  {      
                if (result->resolved)
                {
                    return;
                }
                if (--(result->pending) == 0)
                {
                    /** Keep it a std::exception instead of a std::array */
                    resultPromise.Reject("All promises rejected");
                } });
            }
            return resultPromise;
        }

        static Promise<T> Race(std::vector<Promise<T>> &promises)
        {
            if (promises.empty())
            {
                /** In JS, this will hang. We choose to forbid this. */
                throw std::invalid_argument("Empty promises");
            }
            auto resultPromise = Promise<T>{};
            struct Result
            {
                bool finished{};
            };
            auto result = std::make_shared<Result>();
            result->finished = false;
            for (size_t i = 0; i < promises.size(); i++)
            {
                promises[i].Then([=](const T &value)
                                 {
                if (result->finished)
                {
                    return;
                }
                result->finished = true;
                resultPromise.Resolve(value); });
                promises[i].Catch([=](const std::exception &e)
                                  {      
                if (result->finished)
                {
                    return;
                }
                result->finished = true;
                resultPromise.Reject(e.what()); });
            }
            return resultPromise;
        }

    private:
        std::shared_ptr<State> _state;
    };

    template <>
    struct Promise<void>
    {
        struct State
        {
            bool resolved = false;
            std::exception_ptr exception = nullptr;
            std::coroutine_handle<> callingHandle = nullptr;
            std::function<void()> thenCallback = nullptr;
            std::function<void(const std::exception &)> catchCallback = nullptr;
            void Resolve()
            {
                resolved = true;
                if (callingHandle)
                {
                    callingHandle.resume();
                }
                else if (thenCallback)
                {
                    thenCallback();
                }
            }
            void Reject(const std::exception_ptr &e)
            {
                exception = e;
                if (callingHandle)
                {
                    callingHandle.resume();
                }
                else if (catchCallback)
                {
                    try
                    {
                        std::rethrow_exception(exception);
                    }
                    catch (const std::exception &ex)
                    {
                        catchCallback(ex);
                    }
                }
            }
            void Reject(const std::string &reason)
            {
                Reject(std::make_exception_ptr(std::runtime_error(reason)));
            }
        };

        struct promise_type
        {
            std::shared_ptr<State> state{std::make_shared<State>()};
            Promise<void> get_return_object()
            {
                return Promise{state};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void()
            {
                state->Resolve();
            }
            void unhandled_exception()
            {
                state->Reject(std::current_exception());
            }
        };

        bool await_ready() const
        {
            return _state->resolved || _state->exception != nullptr;
        }

        void await_suspend(std::coroutine_handle<> handle)
        {
            _state->callingHandle = handle;
        }

        void await_resume()
        {
            if (_state->exception != nullptr)
            {
                std::rethrow_exception(_state->exception);
            }
        }

        Promise(std::shared_ptr<State> state)
            : _state(std::move(state))
        {
        }

        Promise()
            : _state(std::make_shared<State>())
        {
        }

        void Resolve(void) const
        {
            _state->Resolve();
        }

        void Reject(const std::exception_ptr &e) const
        {
            _state->Reject(e);
        }

        void Reject(const std::string &reason) const
        {
            _state->Reject(reason);
        }

        void Then(std::function<void()> callback)
        {
            if (_state->callingHandle)
            {
                throw std::runtime_error("Promise is already awaited");
            }
            _state->thenCallback = callback;
            if (_state->resolved)
            {
                _state->thenCallback();
            }
        }

        void Catch(std::function<void(const std::exception &)> callback)
        {
            if (_state->callingHandle)
            {
                throw std::runtime_error("Promise is already awaited");
            }
            _state->catchCallback = callback;
            if (_state->exception != nullptr)
            {
                try
                {
                    std::rethrow_exception(_state->exception);
                }
                catch (const std::exception &e)
                {
                    _state->catchCallback(e);
                }
            }
        }

        static Promise<void> All(std::vector<Promise<void>> &promises)
        {
            if (promises.empty())
            {
                throw std::invalid_argument("Empty promises");
            }
            auto resultPromise = Promise<void>{};
            struct Result
            {
                size_t pending{};
                bool rejected{};
            };
            auto result = std::make_shared<Result>();
            result->pending = promises.size();
            result->rejected = false;
            for (size_t i = 0; i < promises.size(); ++i)
            {
                promises[i].Then([=]()
                                 {
                if (result->rejected)
                {
                    return;
                }
                if (--(result->pending) == 0)
                {
                    resultPromise.Resolve();
                } });
                promises[i].Catch([=](const std::exception &e)
                                  {      
                if (result->rejected)
                {
                    return;
                }
                result->rejected = true;
                resultPromise.Reject(e.what()); });
            }
            return resultPromise;
        }

        static Promise<void> Any(std::vector<Promise<void>> &promises)
        {
            if (promises.empty())
            {
                throw std::invalid_argument("Empty promises");
            }
            auto resultPromise = Promise<void>{};
            struct Result
            {
                size_t pending{};
                bool resolved{};
            };
            auto result = std::make_shared<Result>();
            result->pending = promises.size();
            result->resolved = false;
            for (size_t i = 0; i < promises.size(); ++i)
            {
                promises[i].Then([=]()
                                 {
                if (result->resolved)
                {
                    return;
                }
                result->resolved = true;
                resultPromise.Resolve(); });
                promises[i].Catch([=](const std::exception &)
                                  {      
                if (result->resolved)
                {
                    return;
                }
                if (--(result->pending) == 0)
                {
                    resultPromise.Reject("All promises rejected");
                } });
            }
            return resultPromise;
        }

        static Promise<void> Race(std::vector<Promise<void>> &promises)
        {
            if (promises.empty())
            {
                throw std::invalid_argument("Empty promises");
            }
            auto resultPromise = Promise<void>{};
            struct Result
            {
                bool finished{};
            };
            auto result = std::make_shared<Result>();
            result->finished = false;
            for (size_t i = 0; i < promises.size(); i++)
            {
                promises[i].Then([=]()
                                 {
                if (result->finished)
                {
                    return;
                }
                result->finished = true;
                resultPromise.Resolve(); });
                promises[i].Catch([=](const std::exception &e)
                                  {      
                if (result->finished)
                {
                    return;
                }
                result->finished = true;
                resultPromise.Reject(e.what()); });
            }
            return resultPromise;
        }

    private:
        std::shared_ptr<State> _state;
    };

} // namespace JS
