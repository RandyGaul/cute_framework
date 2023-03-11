[](../header.md ':include')

Atomics are a rather advanced topic in programming. They aren't unique to Cute Framework, but CF does support atomics. You can check out the [API Reference for Atomics](https://randygaul.github.io/cute_framework/#/api_reference?id=atomics) to see all of the available functions.

To fully understand atomics it's best to read some online articles such as [Beej's Article on Atomics](https://beej.us/guide/bgc/html/split/chapter-atomics.html). Briefly: an atomic operation is on that is guaranteed to start and finish in one go. This is super useful when dealing with multiple threads. Knowing that one thread will complete and operation before any other thread can mess with the operation is important to prevent [race conditions](https://stackoverflow.com/questions/34510/what-is-a-race-condition).

Usually atomics are used to implement multithreaded algorithms with very high performance. The primary use-case is to avoid using a [mutex](https://en.cppreference.com/w/cpp/thread/mutex). Cute Framework also has a mutex type called [`CF_Mutex`](https://randygaul.github.io/cute_framework/#/multithreading/cf_mutex). Generally speaking locking a mutex has a significant performance overhead, while the cost of using an atomic is generally much cheaper.

In CF atomics are used internally to implement some thread pool logic, and for circular buffers. Atomics are used very sparingly within CF's implementation, and are a bit of a niche subject. In all likelihood your game won't need to use any atomics whatsoever.
