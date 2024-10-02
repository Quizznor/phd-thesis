from typing import Union, Iterable, Any
from ..binaries import pd
from ..binaries import np
from . import plt
from . import so

import datetime
from matplotlib.colors import Normalize, Colormap
from matplotlib import cm

def __dir__() -> list[str] :
    """spoof dir function for a clean namespace"""

    _globals = globals()
    private_functions = []
    for _global in _globals:
        if _global.startswith('__test'):
            private_functions.append(_global)
        
    del _globals['Union']
    del _globals['Iterable']
    del _globals['Any']
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
        * *x* (``str``)                         : data or name of the column used for the x axis
        * *y* (``str``)                         : data or name of the column used for the y axis
        * *data* (``pd.Dataframe``)             : the dataset for which the box plot is created

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

        if isinstance(scatter_x[0], datetime.datetime):
            is_datetime = True
            scatter_x = np.array([x.timestamp() for x in scatter_x])
        else: is_datetime = False
    else:
        scatter_x, scatter_y = data[x], data[y]

    bins = kwargs.get('bins', 10)
    if isinstance(bins, int):
        bins = np.linspace(0.9*min(scatter_x), 1.1*max(scatter_x), bins+1)

    # split the data into different boxes
    positions = 0.5 * (bins[1:] + bins[:-1])
    binplace = np.digitize(scatter_x, bins)
    boxes = [scatter_y[np.where(binplace == i+1)] for i in range(len(bins)-1)]

    # visualize results
    ax = kwargs.get('ax', plt.gca())
    # color = next(ax._get_lines.prop_cycler)['color']
    color = kwargs.get('c', 'k')
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

    # if is_datetime:
    #     xticks = ax.get_xticks()
    #     fmt = kwargs.get("fmt", "%h %D")
    #     ticklabels = [datetime.datetime.fromtimestamp(x).strftime(fmt) for x in xticks]
    #     ax.set_xticklabels(ticklabels)

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

def shaded_hist(data : Any, cmap : str, **kwargs) -> Normalize :
    """wrapper for the standard plt.hist, which plots the individual bins in a cmap depending on the x-value"""

    def get_outline_kwargs(kwargs) -> dict :
        outline_kwargs = {
            'color' : kwargs.get('c', 'k'),
            'ls' : kwargs.get('ls', 'solid'),
            'lw' : kwargs.get('lw', 1),
            'bins' : kwargs.get('bins', None),
            'histtype' : 'step'
        }

        return outline_kwargs

    # outline
    _, bins, _ = plt.hist(data, **get_outline_kwargs(kwargs))

    # shade
    cmap = plt.get_cmap(cmap)

    norm = kwargs.get('norm', 'linear')
    if isinstance(norm, str):

        vmin = kwargs.get('vmin', np.min(data))
        vmax = kwargs.get('vmax', np.max(data))

        match norm:
            case 'linear':
                norm = Normalize(vmin, vmax, clip=False)
            case 'log':
                from matplotlib.colors import LogNorm
                norm = LogNorm(vmin, vmax, clip=False)
            # ...
            case _:
                raise NameError(f"{norm=} is not a supported option")

    bin_centers = 0.5 * (bins[1:] + bins[:-1])
    _, _, patches = plt.hist(data, bins=bins)
    for x, b in zip(bin_centers, patches):
        plt.setp(b, 'facecolor', cmap(norm(x)))

    return norm

def preliminary(fig : plt.Figure = None, fontsize : float = 60):
    """helper that plots a big, fat 'preliminary' on top of your figure"""
    import matplotlib.patheffects as patheffects
    if fig is None: fig = plt.gcf()

    fig.text(0.5, 0.5, "Preliminary",
         c='red',
         rotation=15,
         horizontalalignment='center',
         verticalalignment='center',
         fontsize=fontsize,
         path_effects=[patheffects.withStroke(foreground='k', linewidth=2)],
         zorder=10000,
    )

def save(fig : plt.Figure = None, path : str = '') -> None :
    base = '/cr/data01/filip/plots/'
    
    fig = fig if fig is not None else plt.gcf()

    import os
    if not os.path.exists(os.path.dirname(base + path)):
        os.system('mkdir -p {base}/{path}')

    fig.savefig(base + path, bbox_inches='tight')

def to_datetime(timestamps : Iterable) -> list[datetime.datetime] :
    try:
        return [datetime.datetime.fromtimestamp(t) for t in timestamps]
    except TypeError:
        return datetime.datetime.fromtimestamp(timestamps)

def gradient(cmap: Colormap, n_points: int) -> list:
    return [cmap(i/n_points) for i in range(n_points)]