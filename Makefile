.PHONY: all build build-release build-debug test clean fmt fmt-check gen_server_errors gen_python_errors gen sdk-python-build sdk-python-install sdk-python-test \
	pre-commit

CLANG_FORMAT ?= clang-format

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
	git ls-files '*.cpp' '*.hh' '*.h' | xargs -r $(CLANG_FORMAT) -i

fmt-check:
	$(CLANG_FORMAT) --version
	git ls-files '*.cpp' '*.hh' '*.h' | xargs -r $(CLANG_FORMAT) --dry-run --Werror

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

sdk-python-test:
	cd src/sdk/python && python3 -m unittest discover -s tests

pre-commit:
	./scripts/pre_commit.sh