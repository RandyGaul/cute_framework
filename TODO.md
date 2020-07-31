
* [ ] docs
	* [ ] big list of all enums, structs, functions, defines
	* [ ] api by category (graphics, collision, file system, audio, net and events, input, utilities, entities and serialization)
	* [ ] fill out pages for initial release
	* [ ] tutorials on common subjects
		* [ ] 1) sprites -> 2) shader -> 3) culling -> 4) animation -> 5) camera (each of these four tutorials builds onto the next)
		* [ ] input and window
		* [ ] entities, serialization, and messaging
		* [ ] collision detection
		* [ ] response, character controllers (reuse player2d)
		* [ ] tile broadphase, DBVH broadphase
		* [ ] chat relay client/server
	* [ ] Articles (not quite tutorials, but a little more than just a document page)
		* [ ] event queue and networked games
		* [ ] integrating Box2D
		* [ ] localization
		* [ ] virtual file system, patching, and distribution
	* [ ] example game
* [ ] [cute](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute_t.md)
	* [ ] executable icon
* [x] audio
	* [x] load wav
	* [x] load ogg
	* [x] stream wav
	* [x] stream ogg
	* [x] stream then switch ogg
	* [x] stream then crossfade ogg
	* [x] music
	* [x] sounds
* [x] window
	* [x] resize/size changed
	* [x] moved
	* [x] focus (mouse/keyboard)
	* [x] minimize/maximize/restore
	* [x] shown/hidden
* [x] file system
	* [x] file io
	* [x] directory/archive mounting
* [ ] graphics
	* [ ] sprite batching
	* [x] shader
	* [x] vertex/index buffers
	* [ ] blend states
	* [ ] render to texture
	* [ ] post_fx on the render texture
	* [x] debug rendering (line, shape, frames)
	* [x] draw calls
	* [x] projection
	* [ ] textures, wrap mode
	* [ ] scissor
	* [ ] stencil
	* [ ] viewport, resizing
	* [x] pixel upscaling
	* [ ] raster font
	* [x] image loading
	* [ ] universal MVP in shaders
	* [x] d3d9
	* [ ] GL 3.2
	* [ ] GLES 2.0
* [ ] camera
	* [ ] track an entity
	* [ ] set position
	* [ ] set destination + lerp time
	* [ ] state machine driven
* [ ] font
* [x] input
	* [x] mouse
		* [x] cursor
	* [x] keyboard
	* [ ] gamepad
	* [ ] text input
	* [ ] drag n drop file
* [x] math
* [x] collision detection
* [x] concurrency
* [x] time
* [ ] net
	* [x] socket
	* [ ] connection handshake
	* [ ] reliability
		* [ ] packet ack and resend
		* [ ] fragmentation and reassembly
	* [ ] relay server
		* [ ] broadcast to all, to all but one, to one
		* [ ] accept new connection
		* [ ] disconnect client
		* [ ] look for timed out clients
		* [ ] thread to pull packets and queue them
		* [ ] poll server packets (deque)
	* [x] security
		* [x] symmetric encryption
		* [x] asymmetric encryption
	* [ ] network simulator
	* [ ] packet loss and RTT estimator
	* [ ] loopback client
* [x] utf8 (for localization support)
* [x] serialization
* [ ] data structures
	* [x] buffer
	* [x] hash table
	* [x] doubly linked list
	* [ ] dbvh
	* [x] string
	* [x] handle table
* [x] allocators
* [ ] ini
* [x] ecs
	* [x] entity and components
	* [x] global state
* [ ] profile
	* [ ] record capture data
	* [ ] render capture data to screen
	* [ ] print capture data
	* [ ] interpolate capture data
* [x] build/distro options
	* [ ] copy + paste all source into project
	* [ ] build shared libs themselves
	* [ ] download prebuilt release folder
	* [x] cmake support
	* [ ] single-file-header packer (still requires shared lib dependencies)
* [x] error
* [x] unit test harness
