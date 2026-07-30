#!/usr/bin/env bash

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
readonly COVERAGE_ROOT="${JSTINE_COVERAGE_DIR:-${PROJECT_ROOT}/build/coverage}"
readonly CONAN_OUTPUT="${COVERAGE_ROOT}/conan"
readonly CMAKE_BUILD="${COVERAGE_ROOT}/cmake"
readonly CONAN_GENERATORS="${CONAN_OUTPUT}/build/Debug/generators"
readonly TOOLCHAIN_FILE="${CONAN_GENERATORS}/conan_toolchain.cmake"
readonly TEST_BINARY="${CMAKE_BUILD}/tests/unit/jstine_unit_tests"
readonly SERVER_OBJECT_DIR="${CMAKE_BUILD}/src/server/CMakeFiles/jstined_lib.dir"
readonly SOURCE_FILTER="${PROJECT_ROOT}/src/server"
readonly IGNORE_REGEX='(^|/)(build|tests|\.conan2)(/|$)'

fail() {
    echo "coverage: $*" >&2
    exit 1
}

find_llvm_tool() {
    local tool_name="$1"
    local compiler_major="$2"
    local tool_path

    if tool_path="$(command -v "${tool_name}-${compiler_major}" 2>/dev/null)"; then
        printf '%s\n' "${tool_path}"
        return
    fi

    if tool_path="$(command -v "${tool_name}" 2>/dev/null)"; then
        printf '%s\n' "${tool_path}"
        return
    fi

    if command -v xcrun >/dev/null 2>&1 &&
        tool_path="$(xcrun --find "${tool_name}" 2>/dev/null)"; then
        printf '%s\n' "${tool_path}"
        return
    fi

    return 1
}

run_tests() {
    ctest --test-dir "${CMAKE_BUILD}" --output-on-failure
}

generate_llvm_coverage() {
    local run_dir="$1"
    local compiler_version="$2"
    local compiler_major="${compiler_version%%.*}"
    local llvm_profdata
    local llvm_cov
    local profile_data="${run_dir}/coverage.profdata"
    local coverage_objects=()

    llvm_profdata="$(find_llvm_tool llvm-profdata "${compiler_major}")" ||
        fail "llvm-profdata ${compiler_major} is required for Clang coverage"
    llvm_cov="$(find_llvm_tool llvm-cov "${compiler_major}")" ||
        fail "llvm-cov ${compiler_major} is required for Clang coverage"

    LLVM_PROFILE_FILE="${run_dir}/jstine-%p.profraw" run_tests

    local raw_profiles=("${run_dir}"/*.profraw)
    [[ -e "${raw_profiles[0]}" ]] ||
        fail "unit tests did not produce LLVM coverage profiles"

    "${llvm_profdata}" merge -sparse "${raw_profiles[@]}" -o "${profile_data}"

    while IFS= read -r object_file; do
        coverage_objects+=("-object=${object_file}")
    done < <(find "${SERVER_OBJECT_DIR}" -type f -name '*.o')
    [[ "${#coverage_objects[@]}" -gt 0 ]] ||
        fail "instrumented server object files were not found"

    "${llvm_cov}" report \
        "${TEST_BINARY}" \
        "${coverage_objects[@]}" \
        -instr-profile="${profile_data}" \
        -ignore-filename-regex="${IGNORE_REGEX}" \
        "${SOURCE_FILTER}" |
        tee "${run_dir}/report.txt"

    "${llvm_cov}" show \
        "${TEST_BINARY}" \
        "${coverage_objects[@]}" \
        -instr-profile="${profile_data}" \
        -ignore-filename-regex="${IGNORE_REGEX}" \
        -format=html \
        -output-dir="${run_dir}/html" \
        -show-line-counts-or-regions \
        -show-branches=count \
        "${SOURCE_FILTER}"

    "${llvm_cov}" export \
        "${TEST_BINARY}" \
        "${coverage_objects[@]}" \
        -instr-profile="${profile_data}" \
        -ignore-filename-regex="${IGNORE_REGEX}" \
        "${SOURCE_FILTER}" >"${run_dir}/coverage.json"
}

generate_gcc_coverage() {
    local run_dir="$1"
    local gcovr

    gcovr="$(command -v gcovr 2>/dev/null)" ||
        fail "gcovr is required for GCC coverage (install the gcovr package)"

    find "${CMAKE_BUILD}" -type f -name '*.gcda' -delete
    run_tests
    mkdir -p "${run_dir}/html"

    "${gcovr}" \
        --root "${PROJECT_ROOT}" \
        --filter "${SOURCE_FILTER}" \
        --object-directory "${CMAKE_BUILD}" \
        --txt "${run_dir}/report.txt"
    cat "${run_dir}/report.txt"

    "${gcovr}" \
        --root "${PROJECT_ROOT}" \
        --filter "${SOURCE_FILTER}" \
        --object-directory "${CMAKE_BUILD}" \
        --html-details "${run_dir}/html/index.html"

    "${gcovr}" \
        --root "${PROJECT_ROOT}" \
        --filter "${SOURCE_FILTER}" \
        --object-directory "${CMAKE_BUILD}" \
        --json "${run_dir}/coverage.json"
}

mkdir -p "${COVERAGE_ROOT}"

conan install "${PROJECT_ROOT}" \
    --output-folder "${CONAN_OUTPUT}" \
    --build=missing \
    -s build_type=Debug \
    -c tools.cmake.cmaketoolchain:user_presets=""

cmake \
    -S "${PROJECT_ROOT}" \
    -B "${CMAKE_BUILD}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DJSTINE_ENABLE_COVERAGE=ON

cmake --build "${CMAKE_BUILD}" --target jstine_unit_tests --parallel

[[ -x "${TEST_BINARY}" ]] ||
    fail "unit-test binary was not created at ${TEST_BINARY}"

readonly TOOLCHAIN_INFO="${CMAKE_BUILD}/jstine-coverage-toolchain.txt"
[[ -f "${TOOLCHAIN_INFO}" ]] ||
    fail "coverage toolchain metadata was not generated"

readonly COMPILER_ID="$(sed -n '1p' "${TOOLCHAIN_INFO}")"
readonly COMPILER_VERSION="$(sed -n '2p' "${TOOLCHAIN_INFO}")"
readonly RUN_DIR="$(mktemp -d "${COVERAGE_ROOT}/run.XXXXXX")"

case "${COMPILER_ID}" in
    AppleClang | Clang)
        generate_llvm_coverage "${RUN_DIR}" "${COMPILER_VERSION}"
        ;;
    GNU)
        generate_gcc_coverage "${RUN_DIR}"
        ;;
    *)
        fail "unsupported coverage compiler: ${COMPILER_ID}"
        ;;
esac

printf '\nCoverage report: %s\n' "${RUN_DIR}/report.txt"
printf 'HTML report:     %s\n' "${RUN_DIR}/html/index.html"
printf 'JSON report:     %s\n' "${RUN_DIR}/coverage.json"
