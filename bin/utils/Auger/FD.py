from ..plotting import plt
from ..binaries import np

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

def PixelPlot(pixel_data, cmap=plt.cm.viridis, hist_bins=50, unit="", cbarlabel='ADC sum', vmin=None, vmax=None):

    from matplotlib.patches import RegularPolygon
    from matplotlib.colors import Normalize
    from matplotlib.cm import ScalarMappable

    fig, (ax1, ax2) = plt.subplots(1, 2)

    vmin = vmin if vmin is not None else np.nanmin(pixel_data)
    vmax = vmax if vmax is not None else np.nanmax(pixel_data)
    norm = Normalize(vmin=vmin, vmax=vmax)
    
    for ipix, pixel in enumerate(pixel_data, 1):

        # determine pixel location
        col = int(np.ceil(ipix / 22.0))
        row = int(ipix - 22 * (col - 1))

        # determine hexagon viewing angle
        centerRow = 35 / 3.0
        elevation_angle = (row - centerRow) * 1.5 * np.sqrt(3) / 2
        centerCol = 10.5 - 0.5 * (row % 2)
        azimuth_angle = (col - centerCol) * 1.5

        hexagon = RegularPolygon(
            (azimuth_angle, elevation_angle),
            numVertices = 6,
            radius = 0.866,
            orientation=np.radians(60),
            facecolor=cmap(norm(pixel)),
            edgecolor='k',
            lw = 1
        )

        ax1.add_patch(hexagon)

    hist_bins = np.linspace(vmin, vmax, hist_bins)
    n, _, _ = ax2.hist(pixel_data, bins = hist_bins, histtype='step', lw=2)
    ax2.text(0.98, 0.98,
             f"$\mu = {np.mean(pixel_data):.2f} {unit}$, $\sigma/\mu = {np.std(pixel_data)/np.mean(pixel_data):.3f}$" + '\n' \
             + fr"$(\uparrow, \downarrow) = ({min(pixel_data):.0f} {unit}, {max(pixel_data):.0f} {unit})$",
             horizontalalignment='right',
             verticalalignment='top',
             transform=ax2.transAxes,
             )
    ax2.set_ylim(0, 1.2 * max(n))

    fig.colorbar(ScalarMappable(norm=norm, cmap=cmap), ax=ax1, orientation='horizontal', label=cbarlabel)

    ax1.set_xlim(-15.8, 15.8)
    ax1.set_ylim(-15.8, 15.8)
    ax1.invert_yaxis()
    ax1.axis('off')
    ax1.set_aspect(20 / 22)

    return ax1, ax2