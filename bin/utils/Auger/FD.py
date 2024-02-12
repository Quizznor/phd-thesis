from ..plotting import plt

def AperturePlot(ax=None, unit='mm', filterStructure=True, zorder=10) -> None:
    """Add aperture, corrector, lens structure of FD telescopes to a given axis"""

    ax = ax if not ax is None else plt.gca()

    if unit == 'mm':
        uf = 1
        ax.set_xlabel(r'$x$ / mm')
        ax.set_ylabel(r'$y$ / mm')
    elif unit == 'm':
        uf = 1e-3
        ax.set_xlabel(r'$x$ / m')
        ax.set_ylabel(r'$y$ / m')
    else:
        raise ValueError('Only m and mm are supported!')
    aperture = plt.Circle((0, 0), 1100 * uf, color='tab:red', fill=False, lw=2.5, zorder=zorder)
    ax.add_artist(aperture)
    corrector = plt.Circle((0, 0), 1700 / 2 * uf, color='k', fill=False, ls='--', zorder=zorder)
    if filterStructure:
        ax.plot([-1030 * uf, 1030 * uf], [765 * uf / 2] * 2, color='grey', alpha=0.2, lw=0.8, zorder=zorder-1, ls='solid')
        ax.plot([-1030 * uf, 1030 * uf], [-765 * uf / 2] * 2, color='grey', alpha=0.2, lw=0.8, zorder=zorder-1, ls='solid')
        ax.plot([-450 * uf / 2] * 2, [-1078 * uf, 1076 * uf], color='grey', alpha=0.2, lw=0.8, zorder=zorder-1, ls='solid')
        ax.plot([450 * uf / 2] * 2, [-1078 * uf, 1076 * uf], color='grey', alpha=0.2, lw=0.8, zorder=zorder-1, ls='solid')

        ax.plot([450 * uf / 2 + 460 * uf] * 2, [-850 * uf, 850 * uf], color='grey', alpha=0.1, lw=0.5, zorder=zorder-1, ls='solid')
        ax.plot([-450 * uf / 2 - 460 * uf] * 2, [-850 * uf, 850 * uf], color='grey', alpha=0.1, lw=0.5, zorder=zorder-1, ls='solid')
    ax.add_artist(corrector)
    ax.set_xlim(-1300 * uf, 1300 * uf)
    ax.set_ylim(-1300 * uf, 1300 * uf)
    ax.set_aspect('equal', 'box')
