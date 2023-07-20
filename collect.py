import csv
import subprocess
from enum import Enum
from dataclasses import dataclass, fields


class Implementation(Enum):
    SEQ = "seq"
    OMP = "omp"
    PTH = "pth"


@dataclass
class Result:
    array_exp: int
    threads: int
    implementation: Implementation
    time: float
    stddev: float


class MonteCarlo:
    def __init__(self, max_exp, max_threads, runs):
        import math

        self.max_exp = max_exp
        self.max_thread_exp = int(math.log2(max_threads))
        self.runs = runs
        filename_postfix = f"{max_exp}me_{max_threads}mt_{runs}runs"
        self.results_path = f"data/data_{filename_postfix}.csv"
        self.plot_path = f"plots/plot_{filename_postfix}.png"

    def run(self):
        print("Running Monte Carlo")

        # execute inside src folder
        subprocess.call(["make"], cwd="src")

        with open(self.results_path, "w") as f:
            writer = csv.DictWriter(f, fieldnames=[f.name for f in fields(Result)])
            writer.writeheader()

        results = []
        for thread_exp in range(self.max_thread_exp + 1):
            # we need the workload to be about X values/thread
            threads = 2**thread_exp
            for array_exp in range(thread_exp + 5, self.max_exp + 1):
                for impl in Implementation:
                    if bool(threads == 1) != bool(impl == Implementation.SEQ):
                        continue
                    print(f"threads: {threads}, array_exp: {array_exp}, impl: {impl.value}")
                    result = self.__simulate(array_exp, threads, impl)
                    results.append(result)
                    print(result.time)

            with open(self.results_path, "a") as f:
                writer = csv.DictWriter(f, fieldnames=[f.name for f in fields(Result)])
                # TODO: kinda hacky
                rows = [r.__dict__ for r in results if r.threads == threads]
                for row in rows:
                    row["implementation"] = row["implementation"].value
                writer.writerows(rows)

        return results

    def __run(self, array_size, threads, impl):
        # ./time_test --grid_size $array_size --impl $impl --num_threads $threads
        process = subprocess.run(["./time_test",
                                  "--num_threads", str(threads),
                                  "--impl", impl.value,
                                  "--grid_size", str(array_size)],
                                 text=True, capture_output=True, cwd="src")
        return process.stdout.strip()

    def __simulate(self, array_exp, threads, impl):
        runs = self.runs
        times = []
        # print(f"Running ./time_test --num_threads {str(threads)} --impl {impl.value} --grid_size {str(2**array_exp)}")
        for _ in range(self.runs):
            time = self.__run(2**array_exp, threads, impl)
            time = float(time)
            assert time > 0, f"{time=} is not positive"
            if time == 0:
                runs -= 1
            times.append(time)
        time = sum(times) / self.runs
        stddev = sum((t - time)**2 for t in times) / self.runs
        return Result(array_exp, threads, impl, time, stddev)

    def load(self):
        from os.path import exists

        # see if file exists
        if not exists(self.results_path):
            raise FileNotFoundError("%s not found, please run simulation first", self.results_path)

        print("Loading from runs.csv")
        with open(self.results_path) as f:
            reader = csv.DictReader(f)
            runs = list(reader)

        for run in runs:
            for key in run:
                if key not in "threads array_exp root".split():
                    run[key] = float(run[key])
                else:
                    run[key] = int(run[key])

        return runs

    def plot(self, total_runs):
        from matplotlib import pyplot as plt

        print("Plotting results")

        # plot time vs array exp with error bars as stddev for if=0 and if=1
        # (different lines)
        # plt.errorbar([r["array_exp"] for r in runs if r["if_count"] == 0],
        #              [r["time"] for r in runs if r["if_count"] == 0],
        #              yerr=[r["stddev"] for r in runs if r["if_count"] == 0],
        #              label="if=0", color="blue", linestyle="dashed")
        # plt.errorbar([r["array_exp"] for r in runs if r["if_count"] == 1],
        #              [r["time"] for r in runs if r["if_count"] == 1],
        #              yerr=[r["stddev"] for r in runs if r["if_count"] == 1],
        #              label="if=1", color="red")
        # plt.xlabel("log2(array size)")
        # plt.ylabel("time (ms)")
        # plt.legend()

        ncols = 3
        if self.max_thread_exp <= 6:
            nrows = 2
        else:
            nrows = 3

        _, axes = plt.subplots(nrows, ncols, figsize=(10, 10))

        for i, thread_exp in enumerate(range(self.max_thread_exp + 1)):
            threads = 2**thread_exp

            run = [r for r in runs if r["threads"] == threads]
            row = i // ncols
            col = i % ncols

            axes[row, col].errorbar([r["array_exp"] for r in run],
                                    [r["time"] for r in run],
                                    yerr=[r["stddev"] for r in run],
                                    linestyle='None', marker='^', label='Custom')

            # axes[0, 0].errorbar(length,
            #                     t_1_mean_2,
            #                     t_1_std_2,
            #                     linestyle='None', marker='^', label='Default')
            axes[row, col].set_title(f"{threads=}")
            axes[row, col].ticklabel_format(style='sci', scilimits=(-3, 3))
            axes[row, col].legend()

        plt.savefig(self.plot_path, dpi=300)


def parse_args():
    from argparse import ArgumentParser
    from multiprocessing import cpu_count

    parser = ArgumentParser(description="Run Monte Carlo simulation")
    parser.add_argument("-mt", "--max-threads", type=int, default=cpu_count(),
                        help="Maximum number of threads to use")
    parser.add_argument("-me", "--max-exp", type=int, default=12,
                        help="Maximum exponent of array size to use")
    # parser.add_argument("-impl", "--implementation", type=Implementation,
    #                     default=Implementation.SEQ, choices=list(Implementation),
    #                     help="Implementation to use")
    parser.add_argument("-r", "--runs", type=int, default=100,
                        help="Number of runs to average over")
    parser.add_argument("-p", "--plot", action="store_true",
                        help="Plot results")

    return parser.parse_args()


def main():
    from os import mkdir
    from os.path import exists

    assert exists("src"), "Please run from project root"

    if not exists("data"):
        mkdir("data")

    args = parse_args()

    sim = MonteCarlo(args.max_exp, args.max_threads, args.runs)
    if input("Run Monte Carlo? (y/n) ") == "y":
        runs = sim.run()
    else:
        runs = sim.load()

    if args.plot:
        if not exists("plots"):
            mkdir("plots")
        sim.plot(runs)


if __name__ == "__main__":
    main()
