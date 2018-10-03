recast.wasm: main/*.cpp recast/*.cpp
	em++ --bind $^ -o recast.js -s MODULARIZE=1 -s 'EXPORT_NAME="Recast"'

.PHONY: clean
clean:
	rm *.js *.wasm
