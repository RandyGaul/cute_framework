# Data Structures

Cute comes packaged with a few useful data structures. Here's the list of them in no particular order.

* [Doubly linked list](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)
* [Hash table](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h)
* [Dictionary](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_dictionary.h)
* [Handle table](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h)
* [Array](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h)
* [Dynamic AABB tree](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)
* [Priority queue](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_priority_queue.h)
* [LRU cache](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_lru_cache.h)
* [Circular buffer](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_circular_buffer.h)
* [Typeless array](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_typeless_array.h)

Rather than documenting every single function on each of these data structures details about each one is listed right here in this document, along with some short notes on why each data stucture might be useful to you with hypothetical examples.

For more in-depth information about data structures the internet already contains a plethora of good resources, so full blown tutorial-esque info won't be found here -- just some quick notes about helpful highlight info.

You might be wondering why things like the array or dictionary data structures exist -- doesn't the C++ standard implement these things already? The main reasons are listed here.

* Cute's data structures run efficiently in debug mode.
* Cute includes minimal files, whereas including a std header will probably pull in 10's of thousands of lines of code, incurring a significant performance hit for every translation unit including those headers.
* Cute's API is better than the std's (in the author's opinion), namely there are *no iterators*.
* Custom allocators are supported with Cute's data structures in a trivial manner. The std's custom allocators reside in a nightmarish bog of overengineered, totally complicated, and of youth-sapping insanity. Hopefully you can see the author's bias by now :)

## Doubly Linked List

[cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h) - This doubly linked list is a special one where the intent is to store the list's nodes intrusively inside of other structs/classes. However, it's written without complicated C++ templates! The key is to use two macros, namely `CUTE_LIST_NODE` and `CUTE_LIST_HOST`. The former converts a pointer from the host pointer to the node, and the latter does the reverse.

### Circular List

This list is circular in nature. This means there should never be a single `NULL` pointer within the list, ever. This simplification makes the implementation of the doubly linked list extra fast by avoid unnecessary branching (if statements).

> An example showing conversion from host to node pointer while looping over a linked list.

```cpp
object_t* object = get_object();
list_node_t* node_ptr = &object.node;
list_node_t* sentinel = node.prev;

do {
	object = CUTE_LIST_HOST(object_t, node, node_ptr);
	printf("%s\n", object->name);
	node_ptr = node_ptr->next;
} while (node_ptr != sentinel);
```

### List begin/end

When you have access to the list itself (a `list_t` instance), looping over the list can make use of some helper functions. This can sometimes be preferred over using a do-while loop with a sentinel pointer (as shown in the above section).

```cpp
list_t* list = get_list();

for (list_node_t* i = list_begin(list); i != list_end(list); i = i->next) {
	object_t* object = CUTE_LIST_HOST(object_t, node, i);
	printf("%s\n", object->name);
}
```

## Hash Table

[cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h) - This is a rather low-level implementation of a hash table. You might be looking for the `dictionary` if you want a templated general purpose hash table.

Cute's hash table operates with `void*` and byte sizes. This hash table is a little special though. Internally it uses an extra level of indirection so it can expose some very useful functions, namely `hashtable_swap`. This means the table can simultaneously act as a sorted array or priority queue!

## Dictionary

[cute_dictionary.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_dictionary.h) - A fairly high-level templated implementation of a hash table. The purpose is for general purpose constant-time lookups. This should probably be used in most cases instead of [cute_hashtable.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_hashtable.h).

## Handle Table

[cute_handle_table.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_handle_table.h) - A small data structure for storing pointers. The idea is to map a pointer to an integer id along with some extra logic for lifetime management. This is mainly here to implement Cute's ECS, but is exposed just case anyone knows what they're doing and wants to use it.

## Array

[cute_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h) - The most important data structure here. A reimagined `std::vector`.

## Dynamic AABB Tree

[cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h) - A complicated but very useful and versatile broad-phase data structure.

## Priority Queue

[cute_priority_queue.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_priority_queue.h) - Mainly here to implement A*, but left here just in case anyone finds it useful.

## LRU Cache

[cute_lru_cache.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_lru_cache.h) - Mainly here to implement the planned feature of an upper bound to texture RAM consumption, but left here since it's a pretty useful data structure to have laying about.

## Circular Buffer

[cute_circular_buffer.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_circular_buffer.h) - A rather low level data structure for communicating between two separate things in a lockless manner. One producer, and one consumer. This is mostly here to implement some of the low level network code (namely pulling UDP packets off of the UDP stack as fast as possible for servers), but is left here in case anyone wants to use it directly.

## Typeless Array

[cute_typeless_array.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_typeless_array.h) - A typeless, as in no templates, version of the `array` class. This was needed to implement some of Cute's ECS, but is left here in case anyone finds a use for it.
