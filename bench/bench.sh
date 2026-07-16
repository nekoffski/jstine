#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  ./bench/bench.sh --test-run NAME --duration SECONDS --proceses COUNT --binary PATH --args "ARGS" --scenario FILE

Options:
  --test-run NAME      Test run name used for bench-logs/NAME.
  --duration SECONDS   Benchmark duration passed to each bench script.
  --proceses COUNT     Number of benchmark processes. --processes is also accepted.
  --binary PATH        Path to the jstined binary.
  --args "ARGS"        Arguments passed to jstined.
  --scenario FILE      Scenario JSON file.
  -h, --help           Show this help.
EOF
}

die() {
    echo "bench.sh: $*" >&2
    exit 1
}

duration=""
processes=""
binary=""
server_args_raw=""
scenario=""
test_run=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --test-run)
            [[ $# -ge 2 ]] || die "--test-run requires a value"
            test_run="$2"
            shift 2
            ;;
        --duration)
            [[ $# -ge 2 ]] || die "--duration requires a value"
            duration="$2"
            shift 2
            ;;
        --proceses|--processes)
            [[ $# -ge 2 ]] || die "$1 requires a value"
            processes="$2"
            shift 2
            ;;
        --binary)
            [[ $# -ge 2 ]] || die "--binary requires a value"
            binary="$2"
            shift 2
            ;;
        --args)
            [[ $# -ge 2 ]] || die "--args requires a value"
            server_args_raw="$2"
            shift 2
            ;;
        --scenario)
            [[ $# -ge 2 ]] || die "--scenario requires a value"
            scenario="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

[[ -n "$test_run" ]] || die "--test-run is required"
[[ -n "$duration" ]] || die "--duration is required"
[[ -n "$processes" ]] || die "--proceses is required"
[[ -n "$binary" ]] || die "--binary is required"
[[ -n "$scenario" ]] || die "--scenario is required"

[[ -x "$binary" ]] || die "binary is not executable: $binary"
[[ -f "$scenario" ]] || die "scenario file does not exist: $scenario"
command -v jq >/dev/null 2>&1 || die "jq is required"
command -v python3 >/dev/null 2>&1 || die "python3 is required"
command -v nc >/dev/null 2>&1 || die "nc is required"

bench_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$bench_dir/.." && pwd)"
log_dir="$repo_root/bench-logs/$test_run"
report_file="$log_dir/benchmark-report.log"

mkdir -p "$log_dir"
{
    echo "Benchmark Run: $test_run"
    echo "====================$(printf "%${#test_run}s" | tr " " "=")"
    echo "  duration    $duration"
    echo "  processes   $processes"
    echo "  binary      $binary"
    echo "  binary args $server_args_raw"
    echo
} >"$report_file"

server_args=()
while IFS= read -r arg; do
    server_args+=("$arg")
done < <(
    python3 -c 'import shlex, sys; [print(arg) for arg in shlex.split(sys.argv[1])]' \
        "$server_args_raw"
)

api_port="9991"

for ((i = 0; i < ${#server_args[@]}; i++)); do
    case "${server_args[$i]}" in
        --config|-c)
            config_path="${server_args[$((i + 1))]:-}"
            if [[ -n "$config_path" && -f "$config_path" ]]; then
                api_port="$(
                    python3 -c '
import sys
import tomllib

with open(sys.argv[1], "rb") as f:
    config = tomllib.load(f)
print(config.get("api", {}).get("port", 9991))
' "$config_path"
                )"
            fi
            ;;
        --config=*)
            config_path="${server_args[$i]#--config=}"
            if [[ -f "$config_path" ]]; then
                api_port="$(
                    python3 -c '
import sys
import tomllib

with open(sys.argv[1], "rb") as f:
    config = tomllib.load(f)
print(config.get("api", {}).get("port", 9991))
' "$config_path"
                )"
            fi
            ;;
        --api-port)
            api_port="${server_args[$((i + 1))]:-$api_port}"
            ;;
        --api-port=*)
            api_port="${server_args[$i]#--api-port=}"
            ;;
    esac
done

server_pid=""

stop_server() {
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" >/dev/null 2>&1; then
        kill "$server_pid" >/dev/null 2>&1 || true
        wait "$server_pid" >/dev/null 2>&1 || true
    fi
    server_pid=""
}

trap stop_server EXIT INT TERM

wait_for_server() {
    local attempts=50
    local delay=0.2

    for _ in $(seq 1 "$attempts"); do
        if ! kill -0 "$server_pid" >/dev/null 2>&1; then
            wait "$server_pid" || true
            die "jstined exited before benchmark started"
        fi
        if nc -z 127.0.0.1 "$api_port" >/dev/null 2>&1; then
            return
        fi
        sleep "$delay"
    done

    die "jstined did not start listening on 127.0.0.1:$api_port"
}

run_benchmark() {
    local bench_name="$1"
    local bench_script="$2"
    local tags="$3"
    local process_metrics_file="$4"
    local bench_path="$bench_dir/$bench_script"

    [[ -f "$bench_path" ]] || die "benchmark script does not exist: $bench_path"

    local cmd=(
        python3 "$bench_path"
        --duration "$duration"
        --processes "$processes"
        --port "$api_port"
        --name "$bench_name"
        --pid "$server_pid"
        --process-metrics-output file
        --process-metrics-file "$process_metrics_file"
    )

    if [[ -n "$tags" ]]; then
        cmd+=(--tags "$tags")
    fi

    echo "==> benchmark: $bench_name ($bench_script)" >&2
    "${cmd[@]}"
}

jq -e '.scripts | type == "array"' "$scenario" >/dev/null \
    || die "scenario must contain a scripts array"

index=0
while IFS= read -r script; do
    index=$((index + 1))
    bench_name="$(jq -r '.name // empty' <<<"$script")"
    bench_script="$(jq -r '.bench // empty' <<<"$script")"
    [[ -n "$bench_name" ]] || bench_name="${bench_script%.py}"
    [[ -n "$bench_script" ]] || die "scenario script entry is missing bench"

    slug="$(
        python3 -c '
import re
import sys

slug = re.sub(r"[^A-Za-z0-9._-]+", "-", sys.argv[1]).strip("-")
print(slug or "benchmark")
' "$bench_name"
    )"
    prefix="$(printf "%02d-%s" "$index" "$slug")"
    server_log_file="$log_dir/$prefix-jstined.log"
    process_metrics_file="$log_dir/$prefix-process.log"

    tags="$(
        jq -r '
            (.tags // {})
            | to_entries
            | map("\(.key)=\(.value)")
            | join(",")
        ' <<<"$script"
    )"

    echo "==> starting jstined: $binary ${server_args[*]}"
    "$binary" "${server_args[@]}" >"$server_log_file" 2>&1 &
    server_pid="$!"

    wait_for_server
    {
        run_benchmark "$bench_name" "$bench_script" "$tags" "$process_metrics_file"
    } >>"$report_file"
    stop_server
done < <(jq -c '.scripts[]' "$scenario")

cat "$report_file"
