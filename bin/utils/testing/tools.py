from typing import Iterable
from itertools import permutations
from time import perf_counter_ns

def __dir__():
    """spoof dir function for a clean namespace"""

    __globals = globals()        
    del __globals['Iterable']
    del __globals['permutations']
    del __globals['perf_counter_ns']

    return __globals

def time_performance(kernels : Iterable[callable], input : callable, n_range : Iterable[int], repeat : int = 10, skip_verification : bool = False) -> dict : 
    """return the runtime for different callables and different inputs, to analyze O(n) performance"""

    performances = {str(kernel).split()[1] : [] for kernel in kernels}
    for n in n_range:
        input_value = input(n)
        results = []

        for kernel in kernels:
            runtime = 0
            for _ in range(repeat - 1):
                start = perf_counter_ns()
                kernel(input_value)
                runtime += (perf_counter_ns() - start) / repeat
            
            start = perf_counter_ns()
            result = kernel(input_value)
            runtime += (perf_counter_ns() - start) / repeat
            performances[str(kernel).split()[1]].append(runtime)
            results.append(result)

        if not skip_verification:
            for a, b in permutations(results, 2):
                try:
                    if a != b: raise ValueError('Kernels do not produce the same results')
                except ValueError:
                    if (a != b).all(): raise ValueError('Kernels do not produce the same results')
        
    return performances