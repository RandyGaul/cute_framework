# CF_Result

Category: [utility](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=utility)  
GitHub: [cute_result.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_result.h)  
---

Information about the result of a function, containing any potential error details.

Struct Members | Description
--- | ---
`int code` | Either 0 for success, or -1 for failure.
`const char* details` | String containing details about any error encountered.

## Remarks

Check if a result is an error or not with [cf_is_error](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_is_error.md).

## Related Pages

[cf_result_success](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result_success.md)  
[cf_is_error](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_is_error.md)  
cf_result_make  
[cf_result_error](https://github.com/RandyGaul/cute_framework/blob/master/docs/utility/cf_result_error.md)  
