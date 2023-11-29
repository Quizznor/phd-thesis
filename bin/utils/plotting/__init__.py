from ..testing import create_stream_logger
import logging, colorlog

plotting_logger = create_stream_logger("utils.plotting")

def set_plt_style(style : str = 'notebook') -> None : 
    """Change the global plotting style based on performance/look"""
    import scienceplots

    if style == 'script':
        plt.style.use(['science', 'ieee'])
        plt.rcParams['text.usetex'] = True
        plt.rcParams['text.latex.preamble'] = \
            r'\usepackage{lipsum}' \
            + r'\usepackage{amsmath}' \
            + r'\usepackage{upgreek}' \
            + r'\usepackage{siunitx}'
    elif style == 'notebook':
        plt.style.use(['science', 'ieee', 'no-latex', 'high-vis'])

    plotting_logger.debug(f'plotting style set to `{style}`.')
    plotting_logger.debug('run set_plt_style() to change style')

import numpy as np
plotting_logger.info('import numpy as np')
import pandas as pd
plotting_logger.info('import pandas as pd')
import matplotlib.pyplot as plt
plotting_logger.info('import matplotlib.pyplot as plt')
import seaborn as sns
plotting_logger.info('import seaborn as sns')

set_plt_style()

del create_stream_logger
from . import tools as plotting
plotting_logger.info('import plotting.tools as plotting')
del tools