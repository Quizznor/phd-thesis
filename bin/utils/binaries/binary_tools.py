from typing import Iterable, Callable
from . import uncertainties
from . import np

def __dir__() -> list[str] :
    """spoof dir function for a clean namespace"""

    _globals = globals()

    return _globals

def kd1d_estimate(samples : Iterable, **kwargs : dict) -> Callable :
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
        bandwidth=kwargs.get('bandwidth', 1.0), 
        algorithm=kwargs.get('algorithm', 'auto'), 
        kernel=kwargs.get('kernel', 'gaussian'),
    )
    kernel_density.fit(np.array(samples)[:, np.newaxis])

    return lambda x: np.exp(kernel_density.score_samples(np.array(x)[:, np.newaxis]))

def progress_bar(step : int, all_steps : int, in_place : bool = False, name : str = '') -> None :
    """print progress of (typically) for loop to stdout, together with time information"""

    import time
    global start_of_progressbar
    step += 1

    if step == all_steps: return "done... =)"

    try:
        _ = start_of_progressbar
    except NameError:
        start_of_progressbar = time.time()

    convert = lambda x : f"{x//3600:02}:{(x%3600)//60:02}:{x%60:02}"

    padding = f' {len(str(all_steps)) + 1}'
    steps_info = f"{step:{padding}}/{all_steps} // {f'{step/all_steps * 1e2:.2f}':>6}%"
    elapsed = int(time.time() - start_of_progressbar)
    per_step = elapsed/step
    estimated = int(per_step * (all_steps - step))

    time_info = f"running {name} since: {convert(elapsed)} // ETA: {convert(estimated)} // {int(per_step * 1e3):}ms/step" + "         "

    print(" || ".join([steps_info, time_info]), end = '\r' if in_place else '\n')

def bootstrap_ci(fctn : callable, popt : list, pcov : list, x_vals : list, ci : int = 1, n_samples : int = 10000) -> np.ndarray :
    """propagate errors of a function given by fitting algorithm via MC bootstrapping"""

    std = [x.std_dev for x in uncertainties.correlated_values(popt, pcov)]
    bootstrap_params = np.array([np.random.normal(x, ci * s_x, n_samples) for x, s_x in zip(popt, std)])

    err_up, err_down = np.zeros((2, len(x_vals)))
    err_up.fill(-np.inf), err_down.fill(np.inf)

    for params in bootstrap_params.T:

        err_up = np.maximum(err_up, fctn(x_vals, *params))
        err_down = np.minimum(err_down, fctn(x_vals, *params))

    return err_up, err_down