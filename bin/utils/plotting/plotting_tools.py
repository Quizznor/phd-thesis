from typing import Union, Iterable
from ..binaries import pd
from ..binaries import np
from . import plt
from . import so

def __dir__() -> list[str] :
    """spoof dir function for a clean namespace"""

    _globals = globals()
    private_functions = []
    for _global in _globals:
        if _global.startswith('__test'):
            private_functions.append(_global)
        
    del _globals['Union']
    del _globals['Iterable']
    del _globals['plt']
    del _globals['so']
    del _globals['pd']
    del _globals['np']

    for fcn in private_functions:
        del _globals[fcn]

    return _globals

def box_series(x : Union[str, Iterable], y : Union[str, Iterable], data : Union[pd.DataFrame, Iterable] = None, **kwargs) -> None :
    """draw a series of box plots for a sequential dataset
    
    Parameters:
        * *x* (``str``)                     : data or name of the column used for the x axis
        * *y* (``str``)                     : data or name of the column used for the y axis
        * *data* (``pd.Dataframe``)         : the dataset for which the box plot is created

    Keywords:
        * *bins* (``int | Iterable``)      = 10 : number of bins, or bin edges for the boxplots
        * *ax* (``plt.Axes``)       = plt.gca() : the axis onto which the objects are drawn
        * *markersize* (``int``)           = 20 : markersize for the accompanying scatterplot
        * *label* (``str``)              = None : legend label for the accompanying scatterplot
        * *analyze_drift* (``bool``)    = False : run a linear regression on the scatter data

    Todo:
        * Add x/y-unit for more beautiful formatting?
    """

    # get the full dataset and construct valid bins
    if data is None:
        scatter_x, scatter_y = np.array(x), np.array(y)
    else:
        scatter_x, scatter_y = data[x], data[y]

    bins = kwargs.get('bins', 10)
    if isinstance(bins, int):
        bins = np.linspace(min(scatter_x), max(scatter_x), bins+1)
    positions = 0.5 * (bins[1:] + bins[:-1])

    # split the data into different boxes
    binplace = np.digitize(scatter_x, bins)
    boxes = [scatter_y[np.where(binplace == i+1)] for i in range(len(bins)-1)]

    # visualize results
    ax = kwargs.get('ax', plt.gca())
    color = next(ax._get_lines.prop_cycler)['color']
    ax.boxplot(boxes,
               positions=positions, 
               widths=np.diff(bins), 
               showfliers=False, 
               manage_ticks=False,
               medianprops={'color':color},
    )
    ax.scatter(scatter_x, scatter_y, 
               label=kwargs.get('label', fr"$\bar{{y}}={np.mean(scatter_y):.2f}\pm{np.std(scatter_y):.2f}$"),
               s=kwargs.get('markersize', 10),
               edgecolors=color,
               linewidths=0.2,
               facecolor='white', 
               alpha=0.4,
    )

    # run a linear regression, if desired
    if kwargs.get('analyze_drift', False):
        popt, pcov = np.polyfit(scatter_x, scatter_y, 1, cov=True)
        gradient = lambda x : np.array([x, np.ones_like(x)])
        
        model = np.poly1d(popt)
        error = lambda x : [np.sqrt( gradient(i).T @ pcov @ gradient(i) ) for i in x]

        X = np.linspace(bins[0], bins[-1], 100)
        ax.plot(X, model(X), color=color, lw=0.4,
                label=fr"$\hat{{y}}\,\approx\,{popt[0]:.2f}\,$x$\,{'+' if popt[1]>0 else ''}{popt[1]:.2f}$")
        ax.fill_between(X, model(X)-error(X), model(X)+error(X), color=color, alpha = 0.3)

def performance_plot(kernels : Iterable[callable], input : callable, n_range : Iterable[int], repeat : int = 100, skip_verification : bool = False) -> None :
    """visualize the results of a runtime performance test of various kernels over an input range defined by n_range"""

    from ..testing.testing_tools import time_performance
    results = time_performance(kernels, input, n_range, repeat=repeat, skip_verification=skip_verification)

    plt.figure()    
    plt.suptitle(f'Performance results, {repeat} runs avg., verify = {not skip_verification}')
    plt.loglog()
    plt.xlabel("Input size")
    plt.ylabel("Runtime / ns")

    for fcn, runtimes in results.items():
        y, delta_y = np.mean(runtimes, axis=1) * 1e9, np.std(runtimes, axis=1) * 1e9
        plt.fill_between(n_range, y-delta_y, y+delta_y, alpha=0.4)
        plt.plot(n_range, y, label=fcn)

    plt.legend()

def bootstrap_ci_interval() -> tuple([Iterable, Iterable]) : 
    raise NotImplementedError

def __test_box_series() -> None :
    fig = plt.figure()
    fig.suptitle('Box series plot')
    y = np.random.normal(size=800)
    box_series(range(800), y)

def __test_performance_plot() -> None :
    def c_(arr):
        return np.c_[arr, arr]

    def stack(arr):
        return np.stack([arr, arr]).T

    def vstack(arr):
        return np.vstack([arr, arr]).T

    def column_stack(arr):
        return np.column_stack([arr, arr])
    
    performance_plot([c_, stack, vstack, column_stack], np.random.rand, [2**k for k in range(20)])


if __name__ == '__main__':
    __test_box_series()
    __test_performance_plot()