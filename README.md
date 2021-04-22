# Welcome to Cute Framework (CF)

Cute Framework (CF for short) is the *cutest* framework available for making 2D games in C/C++. CF comprises of different features, where the various features avoid inter-dependencies. In this way using CF is about picking and choosing which pieces are needed for your game.

# Gettin' all Cute

Setting up an application and getting started is quite easy. Simply visit [the app docs](https://github.com/RandyGaul/cute_framework/tree/master/doc/app), grab the following code snippet for [app_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_make.md), and off you go.

> Creating a window and closing it.

```cpp
#include <cute.h>
using namespace cute;

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480, along with a DirectX 11 context.
	app_t* app = app_make("Fancy Window Title", 50, 50, 640, 480, CUTE_APP_OPTIONS_D3D11_CONTEXT, argv[0]);

	while (app_is_running(app))
	{
		float dt = calc_dt();
		app_update(app, dt);
		// All your game logic and updates go here...
		app_present(app);
	}
	
	app_destroy(app);
	
	return 0;
}
```

# Docs by API Category

Select one of the categories below to learn more about them. Each category contains information about functions, structs, enums, and anything else relevant in the various Cute Framework header files.

[app](https://github.com/RandyGaul/cute_framework/tree/master/doc/app)  
[clipboard](https://github.com/RandyGaul/cute_framework/tree/master/doc/clipboard)  
[window](https://github.com/RandyGaul/cute_framework/tree/master/doc/window)  
[string](https://github.com/RandyGaul/cute_framework/tree/master/doc/string)  
[time](https://github.com/RandyGaul/cute_framework/tree/master/doc/time)  
[graphics](https://github.com/RandyGaul/cute_framework/tree/master/doc/graphics)  
[audio](https://github.com/RandyGaul/cute_framework/tree/master/doc/audio)  
[ecs](https://github.com/RandyGaul/cute_framework/tree/master/doc/ecs)  

# Docs by API List

TODO
