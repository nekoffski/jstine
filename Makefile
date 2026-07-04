.PHONY: all build build-release build-debug test clean fmt gen_server_errors gen

all: build

build: build-release

build-release:
	conan install . --build=missing -s build_type=Release
	cmake --preset conan-release
	cmake --build --preset conan-release

build-debug:
	conan install . --build=missing -s build_type=Debug
	cmake --preset conan-debug
	cmake --build --preset conan-debug

fmt:
	find src -name '*.cpp' -o -name '*.hh' -o -name '*.h' | xargs clang-format -i

test: build-debug
	ctest --preset conan-debug --output-on-failure

clean:
	rm -rf build CMakeUserPresets.json

gen_server_errors:
	./scripts/gen_errors.py ./conf/errors.in ./src/server/core/ErrorCode.hh

gen: gen_server_errors
