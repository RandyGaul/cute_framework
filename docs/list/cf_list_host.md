[](../header.md ':include')

# CF_LIST_HOST

Category: [list](/api_reference?id=list)  
GitHub: [cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)  
---

Converts a pointer to a node to a pointer to its host struct/object.

```cpp
#define CF_LIST_HOST(T, member, ptr) ((T*)((uintptr_t)ptr - CF_OFFSET_OF(T, member)))
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
MyStruct my = CF_LIST_HOST(MyStruct, node, node_ptr);

// Do whatever is needed with the host:
do_stuff(my);
```

## Remarks

This doubly-linked list is intrusive, meaning you place nodes inside of objects. These helper macros are to
easily convert to/from host pointer and node pointer.

## Related Pages

[CF_ListNode](/list/cf_listnode.md)  
[CF_List](/list/cf_list.md)  
[CF_LIST_NODE](/list/cf_list_node.md)  
[cf_list_back](/list/cf_list_back.md)  
[cf_list_init_node](/list/cf_list_init_node.md)  
[cf_list_init](/list/cf_list_init.md)  
[cf_list_push_front](/list/cf_list_push_front.md)  
[cf_list_push_back](/list/cf_list_push_back.md)  
[cf_list_remove](/list/cf_list_remove.md)  
[cf_list_pop_front](/list/cf_list_pop_front.md)  
[cf_list_pop_back](/list/cf_list_pop_back.md)  
[cf_list_empty](/list/cf_list_empty.md)  
[cf_list_begin](/list/cf_list_begin.md)  
[cf_list_end](/list/cf_list_end.md)  
[cf_list_front](/list/cf_list_front.md)  
