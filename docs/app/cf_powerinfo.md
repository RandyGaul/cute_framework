# CF_PowerInfo

Category: [app](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Detailed information about the power level/status of the application.

Struct Members | Description
--- | ---
`CF_PowerState state` | Enumeration [CF_PowerState](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_powerstate.md) for the power state.
`int seconds_left` | The seconds of battery life left. -1 means not running on the battery, or unable to get a valid value.
`int percentage_left` | The percentage of battery life left from 0 to 100. -1 means not running on the battery, or unable to get a valid value.

## Related Pages

[cf_app_power_info](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_power_info.md)  
[cf_power_state_to_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_power_state_to_string.md)  
[CF_PowerState](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_powerstate.md)  
