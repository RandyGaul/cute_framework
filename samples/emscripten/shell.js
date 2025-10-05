const overlayElement = document.getElementById('overlay-wrapper');
const canvasElement = document.getElementById('canvas');
const statusElement = document.getElementById('status');
const progressElement = document.getElementById('progress-bar');

// As a default initial behavior, pop up an alert when webgl context is lost. To make your
// application robust, you may want to override this behavior before shipping!
// See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
canvasElement.addEventListener("webglcontextlost", (e) => {
	alert('WebGL context lost. You will need to reload the page.');
	e.preventDefault();
}, false);

canvasElement.addEventListener("contextmenu", (e) => {
	e.preventDefault();
}, false);

function showOverlay() {
	overlayElement.style.visibility = 'visible';
}

function hideOverlay() {
	overlayElement.style.visibility = 'hidden';
}

function quantizeDevicePixelRatio() {
	// A non-quantized ratio makes everything ugly
	window._emscripten_get_device_pixel_ratio = () => {
		var dpr = window.devicePixelRatio;
		var quantizedDpr = Math.round(dpr * 2) / 2;
		return quantizedDpr;
	};
}

/**
 * Fetch a WASM file with progress tracking and instantiate it.
 * @param {string} wasmUrl - URL of the WASM file.
 * @param {(percent: number) => void} onProgress - Called with 0..100 progress.
 * @param {WebAssembly.Imports} imports - Imports for the WASM module.
 * @returns {Promise<WebAssembly.Instance>} - The instantiated WASM module instance.
 */
async function fetchAndInstantiateWasm(wasmUrl, onProgress = () => {}, imports = {}) {
	const response = await fetch(wasmUrl);
	if (!response.ok) throw new Error(`Failed to fetch ${wasmUrl}`);

	const contentLength = +response.headers.get('Content-Length') || 0;
	const reader = response.body.getReader();
	let loaded = 0;

	// Wrap a ReadableStream to track progress
	const stream = new ReadableStream({
		async start(controller) {
			while (true) {
				const { done, value } = await reader.read();
				if (done) break;
				loaded += value.length;
				if (contentLength) {
					const percent = (loaded / contentLength) * 100;
					onProgress(percent);
				}
				controller.enqueue(value);
			}
			controller.close();
		}
	});

	// Construct a new Response from the tracked stream
	const trackedResponse = new Response(stream, {
		headers: response.headers
	});

	// Instantiate WASM using streaming API
	const result = await WebAssembly.instantiateStreaming(trackedResponse, imports);
	return result.instance;
}

window.Module = {
	preInit: [
		quantizeDevicePixelRatio,
	],
	print(...args) {
		console.log(...args);
	},
	printErr(...args) {
		console.error(...args);
	},
	canvas: canvasElement,
	onAbort() {
		Module.setStatus("Program aborted");
	},
	setStatus(text) {
		console.log(text);
		const m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
		if (m) {
			progressElement.value = (parseInt(m[2]) / parseInt(m[4])) * 100;
		}
		statusElement.innerText = text;
		if (!text) {
			hideOverlay();
		} else {
			showOverlay();
		}
	},
	totalDependencies: 0,
	monitorRunDependencies(left) {
		this.totalDependencies = Math.max(this.totalDependencies, left);
		Module.setStatus(left ? 'Loading... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
		if (left == this.totalDependencies) {
			progressElement.removeAttribute("value");
		} else {
			progressElement.value = (this.totalDependencies - left) / this.totalDependencies * 100;
		}
	},
	instantiateWasm(imports, callback) {
		const onProgress = (percent) => {
			if (percent <= 100) {  // When compression is used, the real size is larger than Content-Length
				Module.setStatus("Downloading " + Math.floor(percent) + "%");
				progressElement.value = percent;
			} else {
				Module.setStatus("Downloading");
				progressElement.removeAttribute("value");
			}
		};
		fetchAndInstantiateWasm(findWasmBinary(), onProgress, imports).then(callback).catch((err) => {
			Module.setStatus("Error while downloading");
			console.error(err);
		});
		return {};
	},
};
Module.setStatus('Loading...');
window.onerror = () => {
	Module.setStatus('Exception thrown, see JavaScript console');
};
