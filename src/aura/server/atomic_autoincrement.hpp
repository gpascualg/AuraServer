#include <atomic>


template <int instance>
class AtomicAutoIncrement
{
public:
    static uint64_t get()
    {
        return ++_last;
    }

private:
    static std::atomic<uint64_t> _last;
};


template <int C>
std::atomic<uint64_t> AtomicAutoIncrement<C>::_last(0);
