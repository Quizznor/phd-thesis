from typing import Union, Iterable
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np

def __dir__():
    """spoof dir function for a clean namespace"""

    __globals = globals()
    private_functions = []
    for __global in __globals:
        if __global.startswith('__test'):
            private_functions.append(__global)
        
    del __globals['Union']
    del __globals['Iterable']
    del __globals['plt']
    del __globals['sns']
    del __globals['pd']
    del __globals['np']

    for fcn in private_functions:
        del __globals[fcn]

    return __globals

def box_series(x : Union[str, Iterable], y : Union[str, Iterable], data : Union[pd.DataFrame, Iterable] = None, **kwargs) -> None :
    """draw a series of box plots for a sequential dataset
    
    Parameters:
        * *x* (``str``)                     : data or name of the column used for the x axis
        * *y* (``str``)                     : data or name of the column used for the y axis
        * *data* (``pd.Dataframe``)         : the dataset for which the box plot is created

    Keywords:
        * *bins* (``int | Iterable``)  = 10 : number of bins, or bin edges for the boxplots
        * *ax* (``plt.Axes``)   = plt.gca() : the axis onto which the objects are drawn
        * *markersize* (``int``)       = 20 : markersize for the accompanying scatterplot
        * *label* (``str``)          = None : legend label for the accompanying scatterplot
    """

    # get the full dataset and construct valid bins
    if data is None:
        scatter_x, scatter_y = x, y
    else:
        scatter_x, scatter_y = data[x], data[y]

    bins = kwargs.get('bins', 10)
    if isinstance(bins, int):
        bins = np.linspace(min(scatter_x), max(scatter_x), bins)
    positions = 0.5 * (bins[1:] + bins[:-1])

    # split the data into different boxes
    binplace = np.digitize(scatter_x, bins)
    boxes = [scatter_y[np.where(binplace == i+1)] for i in range(len(bins)-1)]

    # visualize results
    ax = kwargs.get('ax', plt.gca())
    ax.boxplot(boxes, positions=positions, widths=np.diff(bins), showfliers=False, manage_ticks=False)
    ax.scatter(scatter_x, scatter_y, 
               edgecolors=next(ax._get_lines.prop_cycler)['color'],
               label=kwargs.get('label', None),
               s=kwargs.get('markersize', 0.2),
               facecolor='white', 
               alpha=0.4,
    )

def __test_box_series() -> None :
    fig = plt.figure()
    fig.suptitle('Box series plot')
    y = np.random.normal(size=800)
    box_series(range(800), y)

if __name__ == '__main__':

    __test_box_series(marksize=20)
    plt.show()