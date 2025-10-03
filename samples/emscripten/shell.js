const overlayElement = document.getElementById('overlay');
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
	onRuntimeInitialized() {
	},
};
Module.setStatus('Loading...');
window.onerror = () => {
	Module.setStatus('Exception thrown, see JavaScript console');
};
