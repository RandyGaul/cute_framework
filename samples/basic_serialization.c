#include <cute.h>

int main(int argc, char** argv)
{
	CF_JDoc doc = cf_make_json(NULL, 0);
	CF_JVal root = cf_json_object(doc);
	cf_json_set_root(doc, root);

	cf_json_object_add_float(doc, root, "x", 1.0f);
	cf_json_object_add_float(doc, root, "y", 2.0f);
	cf_json_object_add_float(doc, root, "z", 3.0f);

	char* s = cf_json_to_string(doc);
	printf("%s\n", s);
	sfree(s);

	cf_destroy_json(doc);

	return 0;
}
