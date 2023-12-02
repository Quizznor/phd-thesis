from typing import Iterable, Callable
from . import sk_neighbors
from . import np

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
    kernel_density = sk_neighbors.KernelDensity(
        bandwidth=kwargs.get('bandwidth', 1.0), 
        algorithm=kwargs.get('algorithm', 'auto'), 
        kernel=kwargs.get('kernel', 'gaussian'),
    )
    kernel_density.fit(np.array(samples)[:, np.newaxis])

    return lambda x: np.exp(kernel_density.score_samples(np.array(x)[:, np.newaxis]))