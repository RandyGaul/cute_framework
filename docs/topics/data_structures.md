# Data Structures

Data structures are used as work-horse tools to implement all kinds of features or tools in games. The data structures available include:

- [Map](../api_reference.md#map)
- [Array](../api_reference.md#array)
- [Linked List](../api_reference.md#list)

> [!NOTE]
> CF provides shortform convenience macros for common operations (e.g. `apush`, `afree`, `map_set`, `map_get`). These are simple wrappers around the longform `cf_array_*` and `map_*` functions. The shortform macros are not individually documented but work identically to their longform counterparts.

## Array in C

Arrays operate on typed pointers in C, and automatically grow when necessary, making them _dynamic_ arrays. If you're familiar with [stretchy buffers](https://github.com/creikey/stretchy-buff), this is exactly that.

> Creating a dynamic array, pushing some elements into the array, and freeing it up afterwards.

```cpp
dyna int* a = NULL;
apush(a, 5);
CF_ASSERT(asize(a) == 1);
asetlen(a, 0);
CF_ASSERT(asize(a) == 0);
afree(a);
```

> [!NOTE]
> The [`dyna`](../array/dyna.md) keyword is an _optional_, but encouraged, macro that doesn't actually do anything. It's used to markup the type and make it clear this is a dynamic array, and not just an `int*`.

The array will automatically grow as elements are pushed. Whenever you want to fetch a particular element just use `a[i]` like any other pointer. When done, free up the array with [`cf_array_free`](../array/cf_array_free.md).

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

## Map

A map is used to map a unique key to a specific value. The value can be fetched later very efficiently (in constant time). This makes the map a very popular data structure for general purpose problem solving. Often times maps are used to store unique identifiers for game objects, assets, and provide an easy way to create associations between different sets of data.

In C we use the [`CF_MAP`](../map/cf_map.md) type, while in C++ there's a `Map<T>` class that wraps the C functionality. It contains a very similar API including get/find, insert, remove, etc. We will cover both the C and C++ APIs in this page.

> [!IMPORTANT]
> Since the table itself grows dynamically, values _may not_ store pointers to themselves or other values. All values are stored as [plain old data (POD)](https://stackoverflow.com/questions/146452/what-are-pod-types-in-c), as their location in memory will get shuffled around internally as the map grows.

## CF_MAP in C

[`CF_MAP(T)`](../map/cf_map.md) is a markup macro for a map type. All keys are `uint64_t`. You can store any POD value type. The map grows automatically as entries are added.

```cpp
CF_MAP(CF_V2) pts = NULL;
map_set(pts, 0, cf_v2(3, 5));
map_set(pts, 10, cf_v2(-1, -1));
map_set(pts, -2, cf_v2(0, 0));

// Use `map_get` to fetch values.
CF_V2 a = map_get(pts, 0);
CF_V2 b = map_get(pts, 10);
CF_V2 c = map_get(pts, -2);

// Loop over {key, item} pairs like so:
uint64_t* keys = map_keys(pts);
for (int i = 0; i < map_size(pts); ++i) {
    uint64_t key = keys[i];
    CF_V2 v = pts[i];
    // ...
}

map_free(pts);
```

All keys for `CF_MAP` are `uint64_t`. You can use pointers, integers, chars, etc. as keys. Use `map_set` to insert, `map_get` to fetch, `map_del` to remove, and `map_free` to clean up.

### Strings as Keys

Since `CK_MAP` uses `uint64_t` keys internally we cannot use strings as keys directly. However there's a _highly recommended_ technique using _string interning_ to create stable, unique string references. The [Strings](../topics/strings.md) page has all the string related details. Here is the list of intern functions:

* [`cf_sintern`](../string/cf_sintern.md)
* [`cf_sintern_range`](../string/cf_sintern_range.md)
* [`cf_sinuke`](../string/cf_sinuke.md)

[`cf_sintern`](../string/cf_sintern.md) is the important one. Given a string it will return you a pointer to an identical string, but with a stable and unique pointer. The pointer is unique based on the contents of the string, ensuring only one copy of any string exists. The pointer will be completely immutable, and valid until [`cf_sinuke`](../string/cf_sinuke.md) is called, which cleans up all memory used by the string interning API up to that point.

By interning a string the stable + unique pointer can be used as a globally unique identifier for the string contents itself. We can then cast the pointer to `uint64_t` and use it as a valid map key. The pattern is to take any dynamic string, pass it to [`cf_sintern`](../string/cf_sintern.md), then pass the stable pointer around from there on (but remember, its contents are immutable!).

```cpp
const char* special_name = cf_sintern("Something Special");

const char* name = GetName();
name = cf_sintern(name);
if (name == special_name) { // Valid to compare pointers directly!
	CF_V2 data = map_get(table, (uint64_t)name, CF_V2); // Cast interned pointer to key.
	DoStuff(&data);
}
```

This pattern can be used as a major optimization:

- Only one copy of a string needs to be stored, heavily reducing memory consumption and improving cache locality.
- String to string comparisons can be done by comparing pointers as opposed to the underlying string contents.

There are of course some downsides.

- Intern'd strings are 100% immutable -- you cannot change their contents after creation.
- It's important not to pollute the intern table with many different little strings inside of a loop. For example, when iterating over numbers or counters.
- Since an intern'd string looks just like a normal string, it can be difficult to remember which is which.

## Map in C++

In C++ we have access to `Map<T>`, a wrapper class for the C map API. It contains many similar functions, including insert, get/try_get, keys/items, remove, and count.

```cpp
Map<v2> vecs;
vecs.insert(0, V2(0,0));
vecs.insert(1, V2(10,0));

v2 a = vecs.get(0);
v2 b = vecs.get(1);

v2* a_ptr = vecs.try_get(0);
v2* b_ptr = vecs.try_get(1);
```

> [!IMPORTANT]
> The `get` function will return by value. If a particular key does not exist the returned value is simply a zero'd out element. If instead you want to know if a particular element was found or not, use `try_get` to return a pointer to an element. `NULL` is returned if a particular key is not found.

> [!IMPORTANT]
> Since the table itself grows dynamically, values _may not_ store pointers to themselves or other values. All values are stored as [plain old data (POD)](https://stackoverflow.com/questions/146452/what-are-pod-types-in-c), as their location in memory will get shuffled around internally as the map grows.

## Linked List

The [`Linked List API`](../api_reference.md#list) in C++ implements a [doubly-linked list](https://en.wikipedia.org/wiki/Doubly_linked_list). Linked lists have really fallen out of favor in recent years due to advancements in hardware, but, are still sometimes quite useful for keeping lists of objects.

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

The linked lists themselves are circular. This can be a bit confusing when traversing the list, so helper functions [`cf_list_begin`](../list/cf_list_begin.md) and [`cf_list_end`](../list/cf_list_end.md) are available to simplify traversals.

```cpp
for (CF_Node* n = cf_list_begin(list); n != cf_list_end(list); n = n->next) {
    do_stuff(n);
}
```
