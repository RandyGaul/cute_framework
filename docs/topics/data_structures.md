[](../header.md ':include')

<br>

Data structures are used as work-horse tools to implement all kinds of features or tools in games. The data structures available include:

- [Hashtable/Map](https://randygaul.github.io/cute_framework/#/api_reference?id=hash)
- [Array](https://randygaul.github.io/cute_framework/#/api_reference?id=array)
- [Linked List](https://randygaul.github.io/cute_framework/#/api_reference?id=list)

## Array in C

Arrays operate on typed pointers in C, and automatically grow when necessary, making them _dynamic_ arrays. If you're familiar with [stretchy buffers](https://github.com/creikey/stretchy-buff), this is exactly that.

> Creating a dynamic array, pushing some elements into the array, and freeing it up afterwards.

```cpp
dyna int* a = NULL;
apush(a, 5);
CF_ASSERT(alen(a) == 1);
alen(a)--;
CF_ASSERT(alen(a) == 0);
afree(a);
```

?> The [`dyna`](https://randygaul.github.io/cute_framework/#/array/dyna) keyword is an _optional_, but encouraged, macro that doesn't actuall do anything. It's used to markup the type and make it clear this is a dynamic array, and not just an `int*`.

The array will automatically grow as elements are pushed. Whenever you want to fetch a particular element just use `a[i]` like any other pointer. When done, free up the array with [`afree`](https://randygaul.github.io/cute_framework/#/array/afree).

You may store any kind of element you wish, including structs. However, since the array will grow as necessary elements cannot store pointers to themselves or any other element. Pointers into the array are _not stable_. A good workaround is to instead store indices into the array, and fetch pointers to elements only temporarily.

## Array in C++

In C++ we have access to `Array<T>`. It's basically the same as the C API, except more strongly typed. It has similar functions including push/add, pop, begin/end, reverse, etc.

```cpp
Array<int> values;
values.add(10);
values.add(15);
values.add(20);
for (int i = 0; i < a.count(); ++i) {
	printf("%d\n", a[i]);
}
```

Since the C++ wrapper has a constructor and destructor there's no need to manually call any free function (unlike the C api) -- the destructor cleans up the array's memory resources whenever it gets called.

## Hash Table/Map

A hash table is used to map a unique key to a specific value. The value can be fetched later very efficiently (in constant time). This makes the hash table a very popular data structure for general purpose problem solving. Often times hash tables are used to store unique identifiers for game objects, assets, and provide an easy way to create associations between different sets of data.

In C we use the [Hash API](https://randygaul.github.io/cute_framework/#/api_reference?id=hash), while in C++ there's a `Map<T>` class that wraps the C functionality. It contains a very similar API including get/find, insert, remove, etc. We will cover both the C and C++ APIs in this page.

## htbl in C

The [`htbl`](https://randygaul.github.io/cute_framework/#/hash/htbl) (stands for hashtable) works on a typed pointer, and automatically grows to fit new key/value pairs as necessary. Internally it's implemented with a [stretchy buffer](https://github.com/creikey/stretchy-buff), just like the [Array API in C](https://randygaul.github.io/cute_framework/#/topics/data_structures?id=array). It can store values with unique 64-bit keys.

```cpp
htbl CF_V2* pts = NULL;
hset(pts, 0, cf_v2(3, 5));
hset(pts, 10, cf_v2(-1, -1);
hset(pts, -2, cf_v2(0, 0));

// Use `hget` to fetch values.
CF_V2 a = hget(pts, 0);
CF_V2 b = hget(pts, 10);
CF_V2 c = hget(pts, -2);

// Loop over {key, item} pairs like so:
const uint64_t* keys = hkeys(pts);
for (int i = 0; i < hcount(pts); ++i) {
    uint64_t key = keys[i];
    CF_V2 v = pts[i];
    // ...
}

hfree(pts);
```

All keys for [`htbl`](https://randygaul.github.io/cute_framework/#/hash/htbl) are typecasted to a `uint64_t`. You can use pointers, integers, chars, etc. as keys. Arbitrary values can be passed to the table, including return results from function calls or hard-coded literals (like `10` or `"Strings!"`).

!> **Important Note** Since the table itself grows dynamically, values _may not_ store pointers to themselves or other values. All values are stored as POD (plain-old data), and will be `memcpy`'d or `realloc`'d around as necessary.

### Strings as Keys

Since the [`htbl`](https://randygaul.github.io/cute_framework/#/hash/htbl) typecasts all keys to `uint64_t` internally we cannot use strings as keys, right? Good question! Actually there's a _highly recommended_ technique to deal with strings as keys. The [Strings](https://randygaul.github.io/cute_framework/#/topics/strings) page has all the string related details. We can make use of the _string interning_ functions to create stable, unique string references. Here is the list of intern functions:

* [`sintern`](https://randygaul.github.io/cute_framework/#/string/sintern)
* [`sintern_range`](https://randygaul.github.io/cute_framework/#/string/sintern_range)
* [`silen`](https://randygaul.github.io/cute_framework/#/string/silen)
* [`sivalid`](https://randygaul.github.io/cute_framework/#/string/sivalid)
* [`sinuke`](https://randygaul.github.io/cute_framework/#/string/sinuke)

[`sintern`](https://randygaul.github.io/cute_framework/#/string/sintern) is the important one. Given a string it will return you a pointer to an identical string, but with a stable and unique pointer. The pointer is unique based on the contents of the string, ensuring only one copy of any string exists. The pointer will be completely immutable, and valid until [`sinuke`](https://randygaul.github.io/cute_framework/#/string/sinuke) is called, which cleans up all memory used by the string interning API up to that point.

By interning a string the stable + unique pointer can be used as a globally unique identifier for the string contents itself. We can then insert the pointer to the string into hashtables and use it as a valid lookup key. The pattern is to take any dynamic string, pass it to [`sintern`](https://randygaul.github.io/cute_framework/#/string/sintern), then pass the stable pointer around from there on (but remember, it's contents are immutable!).

```cpp
const char* special_name = sintern("Something Special");

const char* name = GetName();
name = sintern(name);
if (name == special_name) { // Valid to compare pointers directly!
	Data* data = hget(table, name); // Valid to use as key lookup!
	DoStuff(data);
}
```

This pattern can be used as a major optimization:

- Only one copy of a string needs to be stored, heavily reducing memory consumption and improving cache locality.
- String to string comparisons can be done by comparing pointers as opposed to the underlying string contents.

There are of course some downsides.

- Intern'd strings are 100% immutable -- you cannot change their contents after creation.
- It's important not to pollute the intern table with many different little strings inside of a loop. For example, when iterating over numbers or counters.
- Since an intern'd string looks just like a normal string, it can be difficult to remember which is which. You can still use [`sivalid`](https://randygaul.github.io/cute_framework/#/string/sivalid), but this is just a sanity check meant only for debugging.

## Map in C++

In C++ we have access to `Map<T>`, a wrapper class for the [C htbl API](https://randygaul.github.io/cute_framework/#/topics/data_structures?id=hash-tablemap). It contains many similar functions as the C api, including add/insert, get/try_get, keys/vals.

```cpp
Map<v2> vecs;
vecs.add(0, V2(0,0));
vecs.add(1, V2(10,0));

v2 a = vecs.get(0);
v2 b = vecs.get(1);

v2* a_ptr = vecs.try_get(0);
v2* b_ptr = vecs.try_get(1);
```

!> **Important Note** The `get` function will return by value. If a particular key does not exist the returned value is simply a zero'd out element. If instead you want to know if a particular element was found or not, use `try_get` to return a pointer to an element. `NULL` is returned if a particular key is not found.

!> **Important Note** Since the table itself grows dynamically, values _may not_ store pointers to themselves or other values. All values are stored as POD (plain-old data), and will be `memcpy`'d or `realloc`'d around as necessary.

## Linked List

The [`Linked List API`](https://randygaul.github.io/cute_framework/#/api_reference?id=list) in C++ implements a [doubly-linked list](https://en.wikipedia.org/wiki/Doubly_linked_list). Lniked lists have really fallen out of favor in recent years due to advancements in hardware, but, are still sometimes quite useful for keeping lists of objects.

For example, an [LRU cache](https://leetcode.com/problems/lru-cache/) can be implement with a linked list. Usually linked lists are used nowadays to implement other data structures or algorithms, mainly when constant-time list insertion/removal is top priority. CF's linked lists are intrusive, meaning the nodes of a list live within other structs. Macros are used to convert from pointers to structs to a node, or from a pointer to node to the host struct.

> Converting from host to node pointer.

```cpp
struct MyStruct {
    int a;
    float b;
    CF_ListNode node;
};

MyStruct x = get_struct();
CF_ListNode* node = CF_LIST_NODE(MyStruct, node, &x);

// Do whatever is needed with the node.
do_stuff(node);
```

You can put multiple different nodes inside of a single struct, meaning the struct can be inserted into two independent lists simultaneously.

The linked lists themselves are circular. This can be a bit confusing when traversing the list, so helper functions [`cf_list_begin`](https://randygaul.github.io/cute_framework/#/list/cf_list_begin) and [`cf_list_end`](https://randygaul.github.io/cute_framework/#/list/cf_list_end) are available to simplify traversals.

```cpp
for (CF_Node* n = cf_list_begin(list); n != cf_list_end(list); n = n->next) {
    do_stuff(n);
}
```
