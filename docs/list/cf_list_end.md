# cf_list_end | [list](https://github.com/RandyGaul/cute_framework/blob/master/docs/list_readme.md) | [cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)

Returns a pointer to one passed the end of the list (to the dummy node).

```cpp
CF_ListNode* cf_list_end(CF_List* list)
```

Parameters | Description
--- | ---
List | The list.

## Code Example

> Looping over a list with a for loop.

```cpp
for (CF_Node n = cf_list_begin(list); n != cf_list_end(list); n = n->next) {
    do_stuff(n);
}
```

## Remarks

Since the list is circular with a single dummy node it can be confusing to loop over. To help make this simpler, use
[cf_list_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_begin.md) and [cf_list_end](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_end.md) to perform a loop over the list.

## Related Pages

[CF_ListNode](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_listnode.md)  
[CF_List](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list.md)  
[CUTE_LIST_NODE](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cute_list_node.md)  
[CUTE_LIST_HOST](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cute_list_host.md)  
[cf_list_init_node](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_init_node.md)  
[cf_list_init](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_init.md)  
[cf_list_push_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_push_front.md)  
[cf_list_push_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_push_back.md)  
[cf_list_remove](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_remove.md)  
[cf_list_pop_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_pop_front.md)  
[cf_list_pop_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_pop_back.md)  
[cf_list_empty](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_empty.md)  
[cf_list_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_begin.md)  
[cf_list_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_back.md)  
[cf_list_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_front.md)  
