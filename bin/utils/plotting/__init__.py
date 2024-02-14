__all__ = ['plt', 'so', 'set_plt_style', 'plot']

from .. import create_stream_logger
plotting_logger = create_stream_logger("utils.plotting")
del create_stream_logger

def set_plt_style(styles : str = 'single') -> None : 
    """Change the global plotting style based on performance/look"""
    import scienceplots

    opts = styles.split()

    fontsize = 9.5
    labelsize = 13.0
    markersize = 2.0
    use_tex = 'tex' in opts
    if 'single' in opts:
        figuresize = [6.6, 3.3]
    elif 'double' in opts:
        figuresize = [3.3, 2.5]
    elif 'triple' in opts:
        figuresize = [2.2, 2.5] 
    else:
        plotting_logger.warn(f'I dont know what to do with the arguments youve given me: {opts}')
        figuresize = [2.5, 2.5]

    plt.style.use(['science', 'ieee', 'no-latex'] if not use_tex else ['science', 'ieee'])
    plt.rcParams['font.size'] = fontsize
    plotting_logger.debug(f'font size set to {fontsize}')
    plt.rcParams['axes.labelsize'] = labelsize
    plotting_logger.debug(f'label size set to {labelsize}')
    plt.rcParams['figure.figsize'] = figuresize
    plotting_logger.debug(f'figure size set to {figuresize}')
    plt.rcParams['lines.markersize'] = markersize
    plotting_logger.debug(f'markersize set to {markersize}')
    plt.rcParams['text.usetex'] = use_tex
    plt.rcParams['text.latex.preamble'] = \
        r'\usepackage{lipsum}' \
        + r'\usepackage{amsmath}' \
        + r'\usepackage{upgreek}' \
        + r'\usepackage{siunitx}' \
        + r'\DeclareSIUnit\sr{sr}'\
        + r'\DeclareSIUnit\year{yr}'
    plotting_logger.debug(f'usetex set to {use_tex}')

import matplotlib.pyplot as plt
plotting_logger.info('import matplotlib.pyplot as plt')
import seaborn as so
plotting_logger.info('import seaborn as so')
set_plt_style()
from . import plotting_tools as plot
plotting_logger.info('import plotting.tools as plot')
