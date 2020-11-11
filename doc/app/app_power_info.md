# app_power_info

Returns the power information for the application. See remarks for details.

## Syntax

```cpp
power_info_t app_power_info(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.

## Remarks

struct | power_info_t | Description
--- | --- | ---
power_state_t | state | The seconds of battery life left. -1 means not running on the battery, or unable to get a valid value.
int | percentage_left | The percentage of battery life left from 0 to 100. -1 means not running on the battery, or unable to get a valid value.
