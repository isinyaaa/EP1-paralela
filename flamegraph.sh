#!/usr/bin/env bash

# get the flamegraph scripts from https://github.com/brendangregg/FlameGraph
main() {
    local -r output_file="${1:-kernel}.svg"
    local -r frequency="${2:-99}"
    local -r cmd="${*:3}"

    sudo perf record -a -F "$frequency" -g -- "$cmd"
    sudo perf script | ./stackcollapse-perf.pl > out.perf-folded
    ./flamegraph.pl out.perf-folded > "$output_file"
    rm out.perf-folded
}

main "$@"
