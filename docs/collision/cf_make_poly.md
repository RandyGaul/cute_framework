# cf_make_poly | [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Fills out the polygon with values.

```cpp
void cf_make_poly(CF_Poly* p);
```

## Remarks

Runs [cf_hull](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_hull.md) and [cf_norms](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_norms.md), assumes p->verts and p->count are both set to valid values.

## Related Pages

[CF_Poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly.md)  
[cf_hull](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_hull.md)  
[cf_norms](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_norms.md)  
