from typing import Union, Iterable, Any
from ..binaries.binary_tools import kd1d_estimate
from ..binaries import pd
from ..binaries import np
from .. import CONSTANTS
from pathlib import Path
from . import plt
from . import so

import datetime
from matplotlib.colors import Normalize, Colormap
from matplotlib.gridspec import GridSpec
from matplotlib.colors import BoundaryNorm
from matplotlib.cm import ScalarMappable
from matplotlib import dates


def ridgeplot2d(data, **kwargs) -> dict:

    three_sigma_cut = lambda d: d[np.abs(d - np.nanmean(d))/np.nanstd(d) < 3]
    xmin, xmax = kwargs.get("xmin", None), kwargs.get("xmax", None)
    if xmin is None: xmin = np.min(three_sigma_cut(data))
    if xmax is None: xmax = np.max(three_sigma_cut(data))

    cmap = plt.get_cmap(kwargs.get("cmap", "viridis"))
    labels = kwargs.get("labels", [""] * len(data))
    cbar_ticks = kwargs.get("cbar_ticks", [str(i) for i in range(len(data[0]))])
    bandwidth = kwargs.get("bandwidth", 2e-2)
    colors = gradient(cmap, len(data[0]))
    ec = kwargs.get("ec", "w")
    xlabel = kwargs.get("xlabel", "")
    ylabel = kwargs.get("ylabel", "")

    fig = plt.figure()
    gs = GridSpec(len(data), 2, fig, hspace=-0.3, wspace=0.01, width_ratios=[1, 0.04])

    cax = fig.add_subplot(gs[:, 1])
    spines = ["top", "right", "left"]
    X = np.linspace(xmin, xmax, 10000)

    axes = []
    for i, dd in enumerate(data):
        ax = fig.add_subplot(gs[i, 0], 
                             sharex=None if not i else axes[0], 
                             sharey=None if not i else axes[0])

        ax.set_xlim(left=xmin, right=xmax)
        ax.text(0.99, 0.01, labels[i], transform=ax.transAxes, ha="right", va="bottom")
        ax.tick_params(axis="x", bottom=True, top=False, which='both',
                       labelbottom=True if i == len(data)-1 else False)
        [ax.spines[s].set_visible(False) for s in spines]
        plt.setp(ax.get_yticklabels(), visible=False)
        plt.setp(ax.get_yticklines(minor=True), visible=False)
        plt.setp(ax.get_yticklines(minor=False), visible=False)
        ax.patch.set_alpha(0)
        axes.append(ax)

        for (d, c) in zip(dd, colors):
            d = np.array(d)[~np.isnan(d)]
            if not len(d): continue
            
            Y = kd1d_estimate(d, bandwidth=bandwidth * (xmax-xmin))(X)
            ax.plot(X, Y, c=ec, marker="none", ls="solid", lw=0.4, zorder=10)
            fill = ax.fill_between(X, Y, alpha=0.8, color=c, ec="none")

    mappable = ScalarMappable(BoundaryNorm(np.arange(-0.5, len(data[0]) + 0.5, 1), cmap.N), cmap)
    plt.colorbar(mappable, cax=cax, label=ylabel)
    cax.set_yticks(list(range(0, len(data[0]))), cbar_ticks, rotation=90, va="center")
    axes[0].set_ylim(bottom=0)
    axes[-1].set_xlabel(xlabel)

    return fig


def box_series(
    x: Union[str, Iterable],
    y: Union[str, Iterable],
    data: Union[pd.DataFrame, Iterable] = None,
    **kwargs,
) -> None:
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
        else:
            is_datetime = False
    else:
        scatter_x, scatter_y = data[x], data[y]

    bins = kwargs.get("bins", 10)
    if isinstance(bins, int):
        bins = np.linspace(0.9 * min(scatter_x), 1.1 * max(scatter_x), bins + 1)

    # split the data into different boxes
    positions = 0.5 * (bins[1:] + bins[:-1])
    binplace = np.digitize(scatter_x, bins)
    boxes = [scatter_y[np.where(binplace == i + 1)] for i in range(len(bins) - 1)]

    # visualize results
    ax = kwargs.get("ax", plt.gca())
    # color = next(ax._get_lines.prop_cycler)['color']
    color = kwargs.get("c", "k")
    ax.boxplot(
        boxes,
        positions=positions,
        widths=np.diff(bins),
        showfliers=False,
        manage_ticks=False,
        medianprops={"color": color},
    )
    ax.scatter(
        scatter_x,
        scatter_y,
        label=kwargs.get(
            "label", rf"$\bar{{y}}={np.mean(scatter_y):.2f}\pm{np.std(scatter_y):.2f}$"
        ),
        s=kwargs.get("markersize", 10),
        edgecolors=color,
        linewidths=0.2,
        facecolor="white",
        alpha=0.4,
    )

    # run a linear regression, if desired
    if kwargs.get("analyze_drift", False):
        popt, pcov = np.polyfit(scatter_x, scatter_y, 1, cov=True)
        gradient = lambda x: np.array([x, np.ones_like(x)])

        model = np.poly1d(popt)
        error = lambda x: [np.sqrt(gradient(i).T @ pcov @ gradient(i)) for i in x]

        X = np.linspace(bins[0], bins[-1], 100)
        ax.plot(
            X,
            model(X),
            color=color,
            lw=0.4,
            label=rf"$\hat{{y}}\,\approx\,{popt[0]:.2f}\,$x$\,{'+' if popt[1]>0 else ''}{popt[1]:.2f}$",
        )
        ax.fill_between(
            X, model(X) - error(X), model(X) + error(X), color=color, alpha=0.3
        )

    # if is_datetime:
    #     xticks = ax.get_xticks()
    #     fmt = kwargs.get("fmt", "%h %D")
    #     ticklabels = [datetime.datetime.fromtimestamp(x).strftime(fmt) for x in xticks]
    #     ax.set_xticklabels(ticklabels)


def performance_plot(
    kernels: Iterable[callable],
    input: callable,
    n_range: Iterable[int],
    repeat: int = 100,
    skip_verification: bool = False,
) -> None:
    """visualize the results of a runtime performance test of various kernels over an input range defined by n_range"""

    from ..testing.testing_tools import time_performance

    results = time_performance(
        kernels, input, n_range, repeat=repeat, skip_verification=skip_verification
    )

    plt.figure()
    plt.suptitle(
        f"Performance results, {repeat} runs avg., verify = {not skip_verification}"
    )
    plt.loglog()
    plt.xlabel("Input size")
    plt.ylabel("Runtime / ns")

    for fcn, runtimes in results.items():
        y, delta_y = np.mean(runtimes, axis=1) * 1e9, np.std(runtimes, axis=1) * 1e9
        plt.fill_between(n_range, y - delta_y, y + delta_y, alpha=0.4)
        plt.plot(n_range, y, label=fcn)

    plt.legend()


def shaded_hist(data: Any, cmap: str, **kwargs) -> Normalize:
    """wrapper for the standard plt.hist, which plots the individual bins in a cmap depending on the x-value"""

    def get_outline_kwargs(kwargs) -> dict:
        outline_kwargs = {
            "color": kwargs.get("c", "k"),
            "ls": kwargs.get("ls", "solid"),
            "lw": kwargs.get("lw", 1),
            "bins": kwargs.get("bins", None),
            "histtype": "step",
        }

        return outline_kwargs

    # outline
    _, bins, _ = (kwargs.get("ax", plt.gca())).hist(data, **get_outline_kwargs(kwargs))

    # shade
    cmap = plt.get_cmap(cmap)

    norm = kwargs.get("norm", "linear")
    if isinstance(norm, str):

        vmin = kwargs.get("vmin", np.min(data))
        vmax = kwargs.get("vmax", np.max(data))

        if norm == "linear":
            norm = Normalize(vmin, vmax, clip=False)
        elif norm == "log":
            from matplotlib.colors import LogNorm

            norm = LogNorm(vmin, vmax, clip=False)
        else:
            raise NameError(f"{norm=} is not a supported option")

    bin_centers = 0.5 * (bins[1:] + bins[:-1])
    _, _, patches = (kwargs.get("ax", plt.gca())).hist(data, bins=bins)
    for x, b in zip(bin_centers, patches):
        plt.setp(b, "facecolor", cmap(norm(x)))

    return norm


def preliminary(ax: plt.Axes = None, text: str = "Preliminary", fontsize: float = 60):
    """helper that plots a big, fat 'preliminary' on top of your figure"""
    import matplotlib.patheffects as patheffects

    if ax is None:
        ax = plt.gca()

    ax.text(
        0.5,
        0.5,
        text,
        c="red",
        rotation=15,
        horizontalalignment="center",
        verticalalignment="center",
        fontsize=fontsize,
        path_effects=[patheffects.withStroke(foreground="k", linewidth=fontsize / 20)],
        zorder=10000,
    )


def save(fig: plt.Figure = None, path: str = "", **kwargs) -> None:

    full_path = CONSTANTS.PLOT_PATH / f"{path}.png"
    full_path.parents[0].mkdir(parents=True, exist_ok=True)

    fig = fig if fig is not None else plt.gcf()
    fig.savefig(full_path, bbox_inches="tight", **kwargs)


def to_datetime(timestamps: Iterable) -> list[datetime.datetime]:
    try:
        return [datetime.datetime.fromtimestamp(t) for t in timestamps]
    except TypeError:
        return datetime.datetime.fromtimestamp(timestamps)


def gradient(cmap: Colormap, n_points: int) -> list:
    return [cmap(x) for x in np.linspace(0, 1, n_points, endpoint=True)]


def apply_datetime_format(ax: plt.Axes, which: str = "xaxis") -> None:

    locator = dates.AutoDateLocator()
    formatter = dates.ConciseDateFormatter(locator)

    if which == "xaxis":
        ax.xaxis.set_major_formatter(formatter)
    elif which == "yaxis":
        ax.yaxis.set_major_formatter(formatter)
