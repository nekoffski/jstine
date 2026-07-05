.PHONY: all build build-release build-debug test clean fmt gen_server_errors gen_python_errors gen sdk-python-build sdk-python-install

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

gen_python_errors:
	./scripts/gen_errors.py ./conf/errors.in ./src/sdk/python/jstine/errors.py --lang python

gen: gen_server_errors gen_python_errors

sdk-python-build:
	pip wheel ./src/sdk/python --no-deps -w dist/

sdk-python-install:
	pip install -e ./src/sdk/python
