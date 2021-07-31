# app_init_imgui

This function is for anyone who wants to use the [Dear ImGui](https://github.com/ocornut/imgui) library with Cute Framework.

Initializes a valid Dear ImGui context pointer. This should be passed to `ImGui::SetCurrentContext`.

## Syntax

```cpp
ImGuiContext* app_init_imgui(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.

## Return Value

Returns a pointer to the Dear ImGui context. Pass this to `ImGui::SetCurrentContext`.


## Code Example

> Creating a window and closing it.

```cpp
ImGuiContext* imgui_ctx = app_init_imgui(app);
if (imgui_ctx) {
	ImGui::SetCurrentContext(imgui_ctx);
} else {
	window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "Unable to initialize Dear ImGui.");
}
```
