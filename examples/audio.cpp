#include <cute.h>
using namespace cute;

int main()
{
	cute_t* cute = cute_make("Cute Framework Audio Demo", 50, 50, 400, 400, CUTE_OPTIONS_NO_GFX | CUTE_OPTIONS_RESIZABLE);

	while (is_running(cute)) {
	}

	return 0;
}
