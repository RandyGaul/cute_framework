// Redirect handler for old docsify-style URLs to new MkDocs URLs
// Old format: /#/topics/game_loop_and_time?id=fixed-timestep
// New format: /topics/game_loop_and_time/#fixed-timestep
// Old format: /#/coroutine
// New format: /api_reference/#coroutine

(function() {
  'use strict';

  var hash = window.location.hash;

  // Only process if we have a hash starting with #/
  if (!hash || !hash.startsWith('#/')) {
    return;
  }

  // Remove the leading #/
  var path = hash.substring(2);

  // Parse out any ?id= anchor
  var anchor = '';
  var idMatch = path.match(/\?id=([^&]+)/);
  if (idMatch) {
    anchor = '#' + idMatch[1];
    path = path.replace(/\?id=[^&]+/, '');
  }

  // Remove any trailing ? or & left over
  path = path.replace(/[?&]$/, '');

  // Get the base URL (everything before the hash)
  var baseUrl = window.location.origin + window.location.pathname;
  // Remove trailing index.html if present
  baseUrl = baseUrl.replace(/index\.html$/, '');
  // Ensure it ends with /
  if (!baseUrl.endsWith('/')) {
    baseUrl += '/';
  }

  var newUrl;

  // Map old paths to new paths
  if (path.startsWith('topics/')) {
    // Topics stay in /topics/ but use path-based routing
    // Old: #/topics/game_loop_and_time?id=fixed-timestep
    // New: /topics/game_loop_and_time/#fixed-timestep
    newUrl = baseUrl + path + '/' + anchor;
  } else {
    // API items go to /api_reference/#item
    // Old: #/coroutine or #/app/cf_make_app
    // New: /api_reference/#coroutine or /api_reference/#cf_make_app

    // Extract just the item name (last segment) for the anchor
    var segments = path.split('/');
    var itemName = segments[segments.length - 1];

    // If there's already an anchor from ?id=, use that, otherwise use item name
    if (anchor) {
      newUrl = baseUrl + 'api_reference/' + anchor;
    } else {
      newUrl = baseUrl + 'api_reference/#' + itemName;
    }
  }

  // Clean up any double slashes (except in protocol)
  newUrl = newUrl.replace(/([^:])\/+/g, '$1/');

  // Redirect to the new URL
  window.location.replace(newUrl);
})();
