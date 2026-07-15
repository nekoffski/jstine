from __future__ import annotations

from .metrics import Histogram, Metrics
from .process import ProcessMetrics


def print_metrics(metrics: Metrics, duration: float) -> None:
    _print_section("Benchmark")
    _print_rows(
        [
            ("total ops", _format_counter(metrics.total_ops, duration)),
            ("set ops", _format_counter(metrics.total_sets, duration)),
            ("get ops", _format_counter(metrics.total_gets, duration)),
            ("errors", str(metrics.errors)),
        ]
    )

    if metrics.worker_ops:
        print()
        _print_subsection("Workers")
        _print_rows(
            [
                (tag, _format_counter(metrics.worker_ops[tag], duration))
                for tag in sorted(metrics.worker_ops)
            ]
        )

    print()
    _print_subsection("Latency")
    _print_rows(
        [
            ("set", _format_histogram(metrics.set_latencies)),
            ("get", _format_histogram(metrics.get_latencies)),
        ]
    )


def print_parameters(
    host: str,
    port: int,
    duration: float,
    processes: int,
    workers: dict[str, int],
    pid: int | None,
    process_sample_interval: float,
) -> None:
    worker_text = ", ".join(f"{tag}={count}" for tag, count in sorted(workers.items()))
    rows = [
        ("target", f"{host}:{port}"),
        ("duration", f"{duration:.2f}s"),
        ("processes", str(processes)),
        ("workers", worker_text),
    ]
    if pid is not None:
        rows.extend(
            [
                ("server pid", str(pid)),
                ("sample interval", f"{process_sample_interval:.3f}s"),
            ]
        )

    _print_section("Run")
    _print_rows(rows)
    print()


def print_process_metrics(metrics: ProcessMetrics) -> None:
    _print_section("Server Process")
    rows = [
        ("pid", str(metrics.pid)),
        ("samples", f"{metrics.samples} @ {metrics.sample_interval:.3f}s"),
    ]
    if metrics.runtime_s is not None:
        rows.append(("runtime", f"{metrics.runtime_s:.3f}s"))
    if metrics.cpu_percent is not None:
        rows.append(("avg cpu", f"{metrics.cpu_percent:.2f}% of one core"))
    if metrics.num_threads is not None:
        rows.append(("threads", str(metrics.num_threads)))
    if metrics.num_fds is not None:
        rows.append(("fds", str(metrics.num_fds)))
    _print_rows(rows)

    if metrics.cpu_times:
        print()
        _print_subsection("CPU Time")
        _print_rows(
            [
                (_humanize_name(name), f"{value:.3f}s")
                for name, value in sorted(metrics.cpu_times.items())
            ]
        )

    memory_rows = [
        (_humanize_name(name), _format_kib(value))
        for name, value in sorted(metrics.memory_kib.items())
    ]
    memory_rows.append(("peak rss", _format_kib(metrics.peak_rss_kib)))
    print()
    _print_subsection("Memory")
    _print_rows(memory_rows)

    if metrics.context_switches:
        print()
        _print_subsection("Context Switches")
        _print_rows(
            [
                (_humanize_name(name), f"{value:,}")
                for name, value in sorted(metrics.context_switches.items())
            ]
        )

    if metrics.page_faults:
        print()
        _print_subsection("Page Faults")
        _print_rows(
            [
                (_humanize_name(name), f"{value:,}")
                for name, value in sorted(metrics.page_faults.items())
            ]
        )

    if metrics.io:
        print()
        _print_subsection("I/O")
        _print_rows(
            [
                (_humanize_name(name), _format_io(name, value))
                for name, value in sorted(metrics.io.items())
            ]
        )


def _print_section(title: str) -> None:
    print()
    print(title)
    print("-" * len(title))


def _print_subsection(title: str) -> None:
    print(f"- {title}")


def _print_rows(rows: list[tuple[str, str]]) -> None:
    if not rows:
        return

    width = max(len(label) for label, _ in rows)
    for label, value in rows:
        print(f"  {label:<{width}}  {value}")


def _format_counter(value: int, duration: float | None) -> str:
    text = f"{value:,}"
    if duration is not None:
        text += f" ({value / duration:,.2f}/s)"
    return text


def _format_histogram(histogram: Histogram) -> str:
    if histogram.count == 0:
        return "no data"

    average = histogram.total / histogram.count
    return (
        f"avg {_format_seconds(average)}, "
        f"min {_format_seconds(histogram.minimum)}, "
        f"max {_format_seconds(histogram.maximum)}"
    )


def _format_kib(value: int | None) -> str:
    if value is None:
        return "n/a"
    return f"{value} KiB ({value / 1024.0:.2f} MiB)"


def _format_seconds(value: float | None) -> str:
    if value is None:
        return "n/a"
    if value < 0.001:
        return f"{value * 1_000_000:.2f} us"
    if value < 1.0:
        return f"{value * 1_000:.2f} ms"
    return f"{value:.3f} s"


def _format_io(name: str, value: int) -> str:
    if name.endswith("_bytes"):
        return _format_bytes(value)
    return f"{value:,}"


def _format_bytes(value: int) -> str:
    units = ("B", "KiB", "MiB", "GiB")
    scaled = float(value)
    unit = units[0]
    for unit in units:
        if scaled < 1024.0 or unit == units[-1]:
            break
        scaled /= 1024.0
    return f"{scaled:.2f} {unit}"


def _humanize_name(name: str) -> str:
    return name.replace("_", " ")
