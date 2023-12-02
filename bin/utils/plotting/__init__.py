__all__ = ['plt', 'so', 'set_plt_style', 'plot']

from .. import create_stream_logger
plotting_logger = create_stream_logger("utils.plotting")
del create_stream_logger

def set_plt_style(style : str = 'notebook') -> None : 
    """Change the global plotting style based on performance/look"""
    import scienceplots

    if style == 'script':
        fontsize = 8
        figuresize = [3.3, 2.5]
        plt.style.use(['science', 'ieee'])
        plt.rcParams['text.usetex'] = True
        plt.rcParams['text.latex.preamble'] = \
            r'\usepackage{lipsum}' \
            + r'\usepackage{amsmath}' \
            + r'\usepackage{upgreek}' \
            + r'\usepackage{siunitx}'
    elif style == 'notebook':
        fontsize = 4
        figuresize = [2.4, 1.2]
        plt.style.use(['science', 'ieee', 'no-latex', 'high-vis'])
    
    plt.rcParams['font.size'] = fontsize
    plotting_logger.debug(f'font size set to {fontsize}')
    plt.rcParams['figure.figsize'] = figuresize
    plotting_logger.debug(f'figure size set to {figuresize}')
    plt.rcParams['font.size'] = fontsize
    plotting_logger.debug(f'plotting style set to `{style}`.')

import matplotlib.pyplot as plt
plotting_logger.info('import matplotlib.pyplot as plt')
import seaborn as so
plotting_logger.info('import seaborn as sns')
set_plt_style()
from . import plotting_tools as plot
plotting_logger.info('import plotting.tools as plot')