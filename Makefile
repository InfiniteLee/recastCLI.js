recast.wasm: main/*.cpp recast/*.cpp
	em++ --bind $^ -o recast.js -s MODULARIZE=1 -s 'EXPORT_NAME="Recast"' -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1

.PHONY: clean
clean:
	rm *.js *.wasm
