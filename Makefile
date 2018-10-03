# note that -Os breaks the build :(
EMFLAGS=--bind --closure 1 -Os -s MODULARIZE=1 -s 'EXPORT_NAME="Recast"' -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1 -s 'EXPORTED_FUNCTIONS=["_buildNavmesh", "_loadContent", "_loadArray"]'

recast.wasm: main/*.cpp recast/*.cpp
	EMCC_DEBUG=1 em++ ${EMFLAGS} -o recast.js $^

.PHONY: clean
clean:
	rm *.js *.wasm
