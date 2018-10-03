recast.wasm: main/*.cpp recast/*.cpp
	em++ --bind $^ -o recast.js

.PHONY: clean
clean:
	rm *.js *.wasm
