[](../header.md ':include')

# CF_JType

Category: [json](/api_reference?id=json)  
GitHub: [cute_json.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_json.h)  
---

Describes the type of a [CF_JVal](/json/cf_jval.md).

## Values

Enum | Description
--- | ---
JTYPE_NONE | Empty value, representing an uninitialized state. This is not the same as `CF_JTYPE_NULL`.
JTYPE_NULL | Null.
JTYPE_INT | Integer.
JTYPE_FLOAT | Float.
JTYPE_BOOL | Boolean.
JTYPE_STRING | String.
JTYPE_ARRAY | An Array.
JTYPE_OBJECT | A JSON Object.

## Related Pages

[CF_JDoc](/json/cf_jdoc.md)  
[CF_JVal](/json/cf_jval.md)  
[cf_make_json](/json/cf_make_json.md)  
