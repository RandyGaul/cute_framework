# cf_list_init | [list](https://github.com/RandyGaul/cute_framework/blob/master/docs/list_readme.md) | [cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)

Intializes a list.

```cpp
void cf_list_init(CF_List* list)
```

Parameters | Description
--- | ---
list | The list.

## Remarks

As an optimization the list contains a dummy node inside of it. To traverse this list, use [cf_list_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_begin.md) and
[cf_list_end](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_end.md) in a for loop. See [cf_list_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_begin.md) for an example.

## Related Pages

[CF_ListNode](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_listnode.md)  
[CF_List](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list.md)  
[CUTE_LIST_NODE](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cute_list_node.md)  
[CUTE_LIST_HOST](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cute_list_host.md)  
[cf_list_init_node](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_init_node.md)  
[cf_list_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_back.md)  
[cf_list_push_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_push_front.md)  
[cf_list_push_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_push_back.md)  
[cf_list_remove](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_remove.md)  
[cf_list_pop_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_pop_front.md)  
[cf_list_pop_back](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_pop_back.md)  
[cf_list_empty](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_empty.md)  
[cf_list_begin](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_begin.md)  
[cf_list_end](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_end.md)  
[cf_list_front](https://github.com/RandyGaul/cute_framework/blob/master/docs/list/cf_list_front.md)  
