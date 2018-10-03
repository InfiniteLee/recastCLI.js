A fork of [RecastCLI](https://github.com/but0n/recastCLI.js) that builds a subset of its functionality in WebAssembly, so you can build a navigation mesh directly in a browser.

## Building

Requires the [Emscripten SDK](https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html).

``` shell
$ make
```

Will output to recast.wasm and recast.js (the Emscripten-generated wrapper.) See test.html for an example.
