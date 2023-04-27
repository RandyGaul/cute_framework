#include <cute.h>

int main(int argc, char** argv)
{
	const char* string =
		"a = 10,\n"
		"b = 13,\n"
	;
	size_t len = strlen(string);
	
	CF_KeyValue* kv = cf_kv_read((void*)string, len, NULL);
	
	int val;
	cf_kv_key(kv, "a", NULL); cf_kv_val_int32(kv, &val); printf("a was %d\n", val);
	cf_kv_key(kv, "b", NULL); cf_kv_val_int32(kv, &val); printf("b was %d\n", val);
	
	cf_kv_destroy(kv);
	
	return 0;
}
