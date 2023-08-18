# SPDX-FileCopyrightText: Copyright 2023 Denis Glazkov <glazzk.off@mail.ru>
# SPDX-License-Identifier: MIT

import matplotlib.pyplot as plt
import os
import random
import subprocess

from dataclasses import dataclass
from pathlib import Path
from typing import List, Dict


ARRAY_SIZES = [10**power for power in range(3, 8)]
RANDOM_SEEDS = [random.randint(1, 10000) for _ in range(5)]

MEASURE_CPPSORT = "cppsort"
MEASURE_QSORT = "qsort"
MEASURE_QQSORT = "qqsort"
MEASURE_UNDEFINED = -1

COMPILE_SRCS = ["main.cpp"]
COMPILE_ARGS = ["-std=c++17", "-flto", "-O2", "-Wall", "-Werror", "-pedantic"]

PREBUILT_MSVC_X64_BENCHMARK = Path("prebuilt") / Path("benchmark-msvc.exe")

PLOT_OUTPUT_DIR = Path("assets")
PLOT_X_AXIS_SCALE = "log"
PLOT_X_AXIS_LABEL = "array size"
PLOT_Y_AXIS_LABEL = "milliseconds"


@dataclass
class Measure:
    cppsort: int = 0
    qsort: int = 0
    qqsort: int = 0

    def add(self, other):
        self.cppsort += other.cppsort
        self.qsort += other.qsort
        self.qqsort += other.qqsort

    def devide(self, devider):
        self.cppsort /= devider
        self.qsort /= devider
        self.qqsort /= devider


class Benchmark:
    def __init__(self, executable: str, description: str) -> None:
        self.exe = executable
        self.desc = description

    def run(self, size: int, seed: int) -> Measure:
        try:
            cmd = [self.exe]
            env = dict(SIZE=str(size), SEED=str(seed))
            out = subprocess.check_output(cmd, stderr=subprocess.STDOUT, env=env)

            measures = {
                MEASURE_CPPSORT: MEASURE_UNDEFINED,
                MEASURE_QSORT: MEASURE_UNDEFINED,
                MEASURE_QQSORT: MEASURE_UNDEFINED,
            }

            for line in out.decode(encoding="UTF-8").splitlines():
                for key in measures.keys():
                    if line.startswith(f"[{key}]"):
                        measures[key] = int(line.split()[2])

            for key in measures.keys():
                if measures[key] == MEASURE_UNDEFINED:
                    raise RuntimeError(f"Could't get '{measure}' measure")

            return Measure(**measures)
        except subprocess.CalledProcessError as exception:
            raise RuntimeError(exception.stdout.decode(encoding="UTF-8"))


class Compiler:
    def __init__(self, cxx: str, src: List[str], args: List[str], out: str) -> None:
        self.cxx = cxx
        self.src = src
        self.args = args
        self.out = out

    def compile(self, description: str) -> Benchmark:
        try:
            cmd = [self.cxx, *self.src, *self.args, "-o", self.out]
            subprocess.check_output(cmd, stderr=subprocess.STDOUT)

            return Benchmark(self.out, description)
        except subprocess.CalledProcessError as exception:
            raise RuntimeError(exception.stdout.decode(encoding="UTF-8"))


def format_runtime_exception(exception: RuntimeError) -> str:
    errors = map(lambda error: f"  |  {error}", str(exception).splitlines())
    return "\n".join(errors)


def is_windows() -> bool:
    return os.name == "nt"


def save_measures_plot(measures: Dict[str, List[int]], title: str):
    figure, axes = plt.subplots(figsize=(10, 5))

    for key in measures.keys():
        axes.plot(ARRAY_SIZES, measures[key], alpha=0.75, label=key)
        axes.fill_between(ARRAY_SIZES, measures[key], alpha=0.1)

    axes.set_title(title)
    axes.set_xscale(PLOT_X_AXIS_SCALE)
    axes.set_xlabel(PLOT_X_AXIS_LABEL)
    axes.set_ylabel(PLOT_Y_AXIS_LABEL)
    axes.legend()

    os.makedirs(PLOT_OUTPUT_DIR, exist_ok=True)
    figure.savefig(PLOT_OUTPUT_DIR / Path(f"{title}.svg"))


if __name__ == "__main__":
    benchmarks: List[Benchmark] = []

    if is_windows():
        benchmarks.append(Benchmark(str(PREBUILT_MSVC_X64_BENCHMARK), "msvc"))
    else:
        compilers: List[Compiler] = [
            Compiler("g++", COMPILE_SRCS, COMPILE_ARGS, "./benchmark-g++"),
            Compiler("clang++", COMPILE_SRCS, COMPILE_ARGS, "./benchmark-clang++"),
        ]

        for compiler in compilers:
            try:
                benchmarks.append(compiler.compile(compiler.cxx))
            except RuntimeError as exception:
                print(f"ERROR: could't compile source code using '{compiler.cxx}'")
                print(format_runtime_exception(exception))

    for bench in benchmarks:
        measures: Dict[str, List[int]] = {
            MEASURE_CPPSORT: [],
            MEASURE_QSORT: [],
            MEASURE_QQSORT: [],
        }

        for size in ARRAY_SIZES:
            measure = Measure()

            for seed in RANDOM_SEEDS:
                try:
                    measure.add(bench.run(size, seed))
                except RuntimeError as exception:
                    print(f"ERROR: could't measure benchmark for '{compiler.cxx}'")
                    print(format_runtime_exception(exception))

            measure.devide(len(RANDOM_SEEDS))

            measures[MEASURE_CPPSORT].append(measure.cppsort)
            measures[MEASURE_QSORT].append(measure.qsort)
            measures[MEASURE_QQSORT].append(measure.qqsort)

        save_measures_plot(measures, bench.desc)
