

#ifndef TURBO_BASE_SINGLETON_H_
#define TURBO_BASE_SINGLETON_H_

namespace turbo {

    /** @class Singleton

    @brief class template to create a thread-safe singleton object

    */
    template<typename T>
    class Singleton {

    public:

        /**
        @brief get a reference to the singleton object
        */
        inline static T &get() {
            static T instance;
            return instance;
        }

    private:

        Singleton() = default;

        ~Singleton() = default;

        Singleton(const Singleton &) = delete;

        Singleton &operator=(const Singleton &) = delete;
    };


}  // namespace turbo

#endif  // TURBO_BASE_SINGLETON_H_
