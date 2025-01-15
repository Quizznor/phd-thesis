__all__ = ["plt", "so", "set_plt_style", "plot"]

from .. import create_stream_logger

plotting_logger = create_stream_logger("utils.plotting")
del create_stream_logger

import matplotlib.pyplot as plt

plotting_logger.info("import matplotlib.pyplot as plt")
import seaborn as so

plotting_logger.info("import seaborn as so")
from . import plotting_tools as plot

plotting_logger.info("import plotting.tools as plot")


def set_plt_style(styles: str = "single") -> None:
    """Change the global plotting style based on performance/look"""
    import scienceplots

    opts = styles.split()

    fontsize = 9.5
    labelsize = 13.0
    markersize = 2.0

    if "single" in opts:
        figuresize = [6.6, 3.3]
    elif "double" in opts:
        figuresize = [3.3, 2.5]
    elif "triple" in opts:
        figuresize = [2.2, 2.5]
    else:
        plotting_logger.warning(
            f"I dont know what to do with the arguments youve given me: {opts}"
        )
        figuresize = [2.5, 2.5]

    styles = ["science", "ieee"]
    if not "tex" in opts:
        styles += ["no-latex"]

    plt.style.use(styles)
    plt.rcParams["font.size"] = fontsize
    plotting_logger.debug(f"font size set to {fontsize}")
    plt.rcParams["axes.labelsize"] = labelsize
    plotting_logger.debug(f"label size set to {labelsize}")
    plt.rcParams["figure.figsize"] = figuresize
    plotting_logger.debug(f"figure size set to {figuresize}")
    plt.rcParams["lines.markersize"] = markersize
    plotting_logger.debug(f"markersize set to {markersize}")
    plt.rcParams["text.usetex"] = "tex" in opts
    plt.rcParams["text.latex.preamble"] = (
        r"\usepackage{lipsum}"
        + r"\usepackage{amsmath}"
        + r"\usepackage{upgreek}"
        + r"\usepackage{siunitx}"
        + r"\DeclareSIUnit\sr{sr}"
        + r"\DeclareSIUnit\year{yr}"
    )
    plotting_logger.debug(f'usetex set to {"tex" in opts}')

    from matplotlib import cycler

    if "dark" in opts:

        plotting_logger.debug(f"using dark mode!")

        # DARK COLORS
        TEXT_COLOR = "gray"
        BG_COLOR = "#171717"
        colors = ["gray", "r", "steelblue", "g"]

        plt.rcParams["axes.edgecolor"] = TEXT_COLOR
        plt.rcParams["axes.facecolor"] = BG_COLOR
        plt.rcParams["figure.facecolor"] = BG_COLOR
        plt.rcParams["text.color"] = TEXT_COLOR
        plt.rcParams["axes.labelcolor"] = TEXT_COLOR
        plt.rcParams["xtick.color"] = TEXT_COLOR
        plt.rcParams["ytick.color"] = TEXT_COLOR
    else:
        colors = ["k", "r", "b", "g", 'c']

    from matplotlib import cycler

    plt.rcParams["axes.prop_cycle"] = cycler(
        color=colors, ls=["-", "--", ":", "-.",
                          (0, (3, 5, 1, 5))], 
                          marker=["none", "o", "s", "^", "v"]
    )


set_plt_style()
