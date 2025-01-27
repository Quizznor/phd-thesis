from typing import Iterable, Callable, Any
from time import perf_counter_ns
from . import uncertainties
from . import np
import pickle


class ProgressBar:

    def __init__(self, data: Iterable, /, **kwargs) -> None:
        self.print_every = kwargs.get("print_every", 1)
        self.newline = kwargs.get("newline", False)
        self.enum = kwargs.get("enum", -1)
        self.bar_length = kwargs.get("len", 20)
        self.desc = kwargs.get("desc", "")
        if self.desc != "":
            self.desc += ": "

        try:
            self.len = len(data)
            self.data = enumerate(data, self.enum) if self.enum != -1 else iter(data)
        except TypeError:
            from .. import create_stream_logger

            logger = create_stream_logger("ProgressBar")
            logger.warning(
                "Consider using enum=True instead of passing a generator, please"
            )
            self.len = np.inf
            self.data = data

    def __iter__(self) -> "ProgressBar":
        self.start_time = perf_counter_ns()
        self.__index = 0
        return self

    def __next__(self) -> Any:

        self.__index += 1
        if self.__index % self.print_every == 0 and self.__index <= self.len:
            elapsed = perf_counter_ns() - self.start_time
            iterations_per_ns = self.__index / elapsed
            eta_ns = (self.len - self.__index) / iterations_per_ns

            if np.round(iterations_per_s := iterations_per_ns * 1e9, 2) == 0:
                iterations_per_time = f"{iterations_per_s * 3600: >12.2f} it/h"
            else:
                iterations_per_time = f"{iterations_per_s: >12.2f} it/s"

            step_info = (
                f"{self.desc}{self.__index:{len(str(self.len))}}/{self.len} "
                + f"[{'*' * int(self.__index / self.len * self.bar_length): <{self.bar_length}}]"
                + f" || {self.format(elapsed)}>{self.format(eta_ns)}, {iterations_per_time}"
            )
            print(step_info, end=f"\n" if self.newline else "\r")
        elif self.__index > self.len:
            print()

        return next(self.data)

    @staticmethod
    def format(nanoseconds: int) -> str:
        seconds = nanoseconds * 1e-9
        hours = seconds // 3600
        minutes = (seconds % 3600) // 60
        seconds = int(seconds % 60)

        return f"{int(hours):02}:{int(minutes):02}:{int(seconds):02}"


def kd1d_estimate(samples: Iterable, **kwargs: dict) -> Callable:
    """approximate a pdf from an underlaying sample of datapoints

    Parameters:
        * *samples* (``str``)                                                                                        : the samples for which a PDF is estimated via KDE

    Keywords:
        * *bandwidth* (``float | {'scott' | 'silverman'}``)                                                      = 1 : parameter that determines the function smoothness
        * *algorithm* (``{'kd_tree' | 'ball_tree' | 'auto'}``)                                                = auto : tree algorithm to estimate the kernel density
        * *kernel* (``{'gaussian' | 'tophat' | 'epanechnikov' | 'exponential' | 'linear' | 'cosine'}``) = 'gaussian' : the kernel function

    Todo:
        * implement this in Nd-case?
    """

    from sklearn import neighbors

    kernel_density = neighbors.KernelDensity(
        bandwidth=kwargs.get("bandwidth", 1.0),
        algorithm=kwargs.get("algorithm", "auto"),
        kernel=kwargs.get("kernel", "gaussian"),
    )
    kernel_density.fit(np.array(samples)[:, np.newaxis])

    return lambda x: np.exp(kernel_density.score_samples(np.array(x)[:, np.newaxis]))


def progress_bar(
    step: int, all_steps: int, in_place: bool = False, name: str = ""
) -> None:
    """print progress of (typically) for loop to stdout, together with time information"""

    import time

    global start_of_progressbar
    step += 1

    if step == all_steps:
        return "done... =)"

    try:
        _ = start_of_progressbar
    except NameError:
        start_of_progressbar = time.time()

    convert = lambda x: f"{x//3600:02}:{(x%3600)//60:02}:{x%60:02}"

    padding = f" {len(str(all_steps)) + 1}"
    steps_info = f"{step:{padding}}/{all_steps} // {f'{step/all_steps * 1e2:.2f}':>6}%"
    elapsed = int(time.time() - start_of_progressbar)
    per_step = elapsed / step
    estimated = int(per_step * (all_steps - step))

    time_info = (
        f"running {name} since: {convert(elapsed)} // ETA: {convert(estimated)} // {int(per_step * 1e3):}ms/step"
        + "         "
    )

    print(" || ".join([steps_info, time_info]), end="\r" if in_place else "\n")


def bootstrap_ci(
    fctn: callable,
    popt: list,
    pcov: list,
    x_vals: list,
    ci: int = 1,
    n_samples: int = 10000,
) -> np.ndarray:
    """propagate errors of a function given by fitting algorithm via MC bootstrapping"""

    std = [x.std_dev for x in uncertainties.correlated_values(popt, pcov)]
    bootstrap_params = np.array(
        [np.random.normal(x, ci * s_x, n_samples) for x, s_x in zip(popt, std)]
    )

    err_up, err_down = np.zeros((2, len(x_vals)))
    err_up.fill(-np.inf), err_down.fill(np.inf)

    for params in bootstrap_params.T:

        err_up = np.maximum(err_up, fctn(x_vals, *params))
        err_down = np.minimum(err_down, fctn(x_vals, *params))

    return err_up, err_down


def closest(array: list[Any], value, index=True) -> Any:
    """return the element closest to value from a collection of values in array"""
    closest_index = np.argmin(np.abs(np.array(array) - value))
    return closest_index if index else array[closest_index]


def pickle_save(path_to_file: str, obj: Any) -> None:
    with open(path_to_file, "wb") as f:
        pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)


def pickle_load(path_to_file: str) -> Any:
    with open(path_to_file, "rb") as f:
        data = pickle.load(f)

    return data