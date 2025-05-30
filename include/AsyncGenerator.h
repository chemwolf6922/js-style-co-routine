#pragma once

#include "Promise.h"
#include <queue>

namespace JS
{
    template <typename T>
    struct AsyncGenerator
    {
        struct State
        {
            std::queue<T> values{};
            std::exception_ptr exception{nullptr};
            std::optional<Promise<std::optional<T>>> nextPromise{};
            bool finished{false};

            Promise<std::optional<T>> NextAsync()
            {
                Promise<std::optional<T>> promise{};
                if (!values.empty())
                {
                    auto v = std::move(values.front());
                    values.pop();
                    promise.Resolve(std::make_optional<T>(std::move(v)));
                }
                else if (exception)
                {
                    promise.Reject(exception);
                    exception = nullptr;
                }
                else if (finished)
                {
                    promise.Resolve(std::optional<T>());
                }
                else if (nextPromise.has_value())
                {
                    promise.Reject("Overlapping Next calls are not allowed");
                }
                else
                {
                    nextPromise = promise;
                }
                return promise;
            }

            void Feed(T&& v)
            {
                if (nextPromise.has_value())
                {
                    auto promise = nextPromise.value();
                    nextPromise.reset();
                    promise.Resolve(std::make_optional<T>(std::move(v)));
                }
                else
                {
                    values.push(std::move(v));
                }
            }

            void Feed(const T &v)
            {
                if (nextPromise.has_value())
                {
                    auto promise = nextPromise.value();
                    nextPromise.reset();
                    promise.Resolve(std::make_optional<T>(v));
                }
                else
                {
                    values.push(v);
                }
            }

            void Finish()
            {
                finished = true;
                if (nextPromise.has_value())
                {
                    auto promise = nextPromise.value();
                    nextPromise.reset();
                    promise.Resolve(std::optional<T>());
                }
            }

            void Reject(const std::exception_ptr &e)
            {
                finished = true;
                if (nextPromise.has_value())
                {
                    auto promise = nextPromise.value();
                    nextPromise.reset();
                    promise.Reject(e);
                }
                else
                {
                    exception = e;
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
            AsyncGenerator<T> get_return_object()
            {
                return AsyncGenerator{state};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never yield_value(T &&v)
            {
                state->Feed(std::move(v));
                return {};
            }
            std::suspend_never yield_value(const T &v)
            {
                state->Feed(v);
                return {};
            }
            void return_void() {}
            std::suspend_never final_suspend() noexcept
            {
                state->Finish();
                return {};
            }
            void unhandled_exception()
            {
                state->Reject(std::current_exception());
            }
        };

        AsyncGenerator(std::shared_ptr<State> state)
            : _state(std::move(state))
        {
        }

        AsyncGenerator()
            : _state(std::make_shared<State>())
        {
        }

        Promise<std::optional<T>> NextAsync() const
        {
            return _state->NextAsync();
        }

        void Feed(T &&value) const
        {
            _state->Feed(std::move(value));
        }

        void Feed(const T &value) const
        {
            _state->Feed(value);
        }

        void Finish() const
        {
            _state->Finish();
        }

        void Reject(const std::exception_ptr &e) const
        {
            _state->Reject(e);
        }

        void Reject(const std::string &reason) const
        {
            _state->Reject(reason);
        }

    private:
        std::shared_ptr<State> _state;
    };

    // No specialization for void, as it does not make sense to yield void values.

} // namespace JS
