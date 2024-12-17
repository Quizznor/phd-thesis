from typing import Iterable, Callable
from . import timeit


def __dir__():
    """spoof dir function for a clean namespace"""

    __globals = globals()
    del __globals["Iterable"]
    del __globals["perf_counter_ns"]

    return __globals


def time_performance(
    kernels: Iterable[Callable],
    input: Callable,
    n_range: Iterable[int],
    repeat: int = 10000,
    skip_verification: bool = False,
) -> dict:
    """return the runtime for different callables and different inputs, to analyze O(n) performance"""

    performances = {str(kernel).split()[1]: [[] for _ in n_range] for kernel in kernels}
    for i_n, n in enumerate(n_range):
        input_value = input(n)
        results = []

        for i_k, kernel in enumerate(kernels):

            t = timeit.Timer(lambda: kernel(input))
            for _ in range(int(repeat**1 / 2)):
                runtime, this_rv = t.timeit(number=int(repeat**1 / 2))
                performances[str(kernel).split()[1]][i_n].append(runtime)

                if not skip_verification:
                    if i_k == 0:
                        last_rv = this_rv
                        continue

                    error_string = f"Return values do not match for {str(kernel).split()[1]} and {str(input).split()[1]}({n})"

                    try:
                        assert last_rv == this_rv, error_string
                    except ValueError:
                        assert (last_rv == this_rv).all(), error_string

                    last_rv = this_rv

    return performances
