#include <cute_app.h>
#include <cute_handle_table.h>
using namespace cute;

int main()
{
	handle_table_t table;
	handle_table_init(&table, 1024, NULL);

	handle_t h0 = handle_table_alloc(&table, 7);
	handle_t h1 = handle_table_alloc(&table, 13);
	int index0 = CUTE_HANDLE_INDEX(h0);
	int index1 = CUTE_HANDLE_INDEX(h1);

	handle_table_free(&table, h0);
	handle_table_free(&table, h1);

	h0 = handle_table_alloc(&table, 4);
	h1 = handle_table_alloc(&table, 267);
	index0 = CUTE_HANDLE_INDEX(h0);
	index1 = CUTE_HANDLE_INDEX(h1);

	h1 = handle_table_update_index(&table, h1, 9);
	index1 = CUTE_HANDLE_INDEX(h1);

	handle_table_clean_up(&table);

	app_t* app = app_make("Cute Framework Audio Demo", 50, 50, 400, 400, APP_OPTIONS_NO_GFX | APP_OPTIONS_RESIZABLE);

	while (is_running(app)) {
	}

	return 0;
}
