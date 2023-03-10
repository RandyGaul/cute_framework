[](../header.md ':include')

# cf_list_init

Category: [list](/api_reference?id=list)  
GitHub: [cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)  
---

Intializes a list.

```cpp
CF_INLINE void cf_list_init(CF_List* list)
```

Parameters | Description
--- | ---
list | The list.

## Remarks

As an optimization the list contains a dummy node inside of it. To traverse this list, use [cf_list_begin](/list/cf_list_begin.md) and
[cf_list_end](/list/cf_list_end.md) in a for loop. See [cf_list_begin](/list/cf_list_begin.md) for an example.

## Related Pages

[CF_ListNode](/list/cf_listnode.md)  
[CF_List](/list/cf_list.md)  
[CF_LIST_NODE](/list/cf_list_node.md)  
[CF_LIST_HOST](/list/cf_list_host.md)  
[cf_list_init_node](/list/cf_list_init_node.md)  
[cf_list_back](/list/cf_list_back.md)  
[cf_list_push_front](/list/cf_list_push_front.md)  
[cf_list_push_back](/list/cf_list_push_back.md)  
[cf_list_remove](/list/cf_list_remove.md)  
[cf_list_pop_front](/list/cf_list_pop_front.md)  
[cf_list_pop_back](/list/cf_list_pop_back.md)  
[cf_list_empty](/list/cf_list_empty.md)  
[cf_list_begin](/list/cf_list_begin.md)  
[cf_list_end](/list/cf_list_end.md)  
[cf_list_front](/list/cf_list_front.md)  
