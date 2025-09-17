# Random Numbers

You may already know about the [`rand`](https://en.cppreference.com/w/c/numeric/random/rand) function in C to generate random numbers. This is a great starting point, as it returns a random number from 0 to `RAND_MAX`. However, there's no guarantee the quality of the random numbers is any good, and also refers to global state, meaning you can only have a single random number generator. Instead, CF provides it's own method for generating random numbers that overcomes these issues.

## Seeding

To initialize a fresh random number generator, [`CF_Rnd`](../random/cf_rnd.md), call [`cf_rnd_seed`](../random/cf_rnd_seed.md).

```cpp
CF_Rnd rnd = cf_rnd_seed(0);
```

The seed acts as the initial parameter for the random number generator, and dictates what number sequence will be generated. Each seed produces a deterministic set of numbers. That means if ever need to recreate something generated from random numbers, you can use the same initial seed.

If we want to generate _seemingly_ random but different numbers each time the application is started up, a great way is to query the system's time. In C there's a function called [`time`](https://en.cppreference.com/w/c/chrono/time) that returns an integer representing the number of seconds elapsed since [_the epoch_](https://en.wikipedia.org/wiki/Epoch_(computing)). You may typecast the return value to an integer and pass it into [`cf_rnd_seed`](../random/cf_rnd_seed.md).

```cpp
#include <time.h>

//

CF_Rnd rnd = cf_rnd_seed((int)(time(NULL));
```

[`CF_Rnd`](../random/cf_rnd.md) only takes up 128 bytes of stack space, so feel free to create as many of them as you need!

## Random Numbers

[`CF_Rnd`](../random/cf_rnd.md) can generate random integers and floats, and also generate them within specified ranges. For example, you can make a function to return a random float from 0 to 1:

```cpp
float random01(CF_Rnd* rnd)
{
	return cf_rnd_range_float(rnd, 0.0f, 1.0f);
}
```
