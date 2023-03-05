# CUTE_LIST_HOST | [list](https://github.com/RandyGaul/cute_framework/blob/master/docs/list_readme.md) | [cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)

Converts a pointer to a node to a pointer to its host struct/object.

```cpp
#define CUTE_LIST_HOST(T, member, ptr) ((T*)((uintptr_t)ptr - CUTE_OFFSET_OF(T, member)))
```

Parameters | Description
--- | ---
T | The type of the host.
member | The name of the host member.
ptr | A pointer to the host.

## Return Value

Returns a pointer to a the host struct/object of type `T`.

## Code Example

> Converting from node to host pointer.

```cpp
struct MyStruct {
    int a;
    float b;
    CF_ListNode node;
};

CF_ListNode node_ptr = get_node();
MyStruct my = CUTE_LIST_HOST(MyStruct, node, node_ptr);

// Do whatever is needed with the host:
do_stuff(my);
```

## Remarks

This doubly-linked list is intrusive, meaning you place nodes inside of objects. These helper macros are to
easily convert to/from host pointer and node pointer.

## Related Pages

[CF_ListNode](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_listnode.md)  
[CF_List](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list.md)  
[CUTE_LIST_NODE](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cute_list_node.md)  
[cf_list_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_back.md)  
[cf_list_init_node](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_init_node.md)  
[cf_list_init](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_init.md)  
[cf_list_push_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_push_front.md)  
[cf_list_push_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_push_back.md)  
[cf_list_remove](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_remove.md)  
[cf_list_pop_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_pop_front.md)  
[cf_list_pop_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_pop_back.md)  
[cf_list_empty](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_empty.md)  
[cf_list_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_begin.md)  
[cf_list_end](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_end.md)  
[cf_list_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_front.md)  
