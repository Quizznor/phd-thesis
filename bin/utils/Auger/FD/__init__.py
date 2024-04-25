from ...plotting import plt
from ...binaries import np

def AperturePlot(ax=None, filterStructure=True) -> plt.axes :
    """Add aperture, corrector, lens structure of FD telescopes to a given axis"""

    ax = ax if not ax is None else plt.gca()

    aperture = plt.Circle((0, 0), 1100, color='tab:red', fill=False, lw=2.5, zorder=1)
    ax.add_artist(aperture)
    corrector = plt.Circle((0, 0), 1700 / 2, color='k', fill=False, ls='--', zorder=1)
    
    if filterStructure:
        ax.plot([-1030, 1030], [765/ 2] * 2, color='grey', alpha=0.2, lw=0.8, zorder=0, ls='solid')
        ax.plot([-1030, 1030], [-765/2] * 2, color='grey', alpha=0.2, lw=0.8, zorder=0, ls='solid')

        ax.plot([-450/ 2] * 2, [-1078, 1076], color='grey', alpha=0.2, lw=0.8, zorder=0, ls='solid')
        ax.plot([450/ 2] * 2, [-1078, 1076], color='grey', alpha=0.2, lw=0.8, zorder=0, ls='solid')
        ax.plot([450/ 2 + 460] * 2, [-850, 850], color='grey', alpha=0.1, lw=0.5, zorder=0, ls='solid')
        ax.plot([-450/ 2 - 460] * 2, [-850, 850], color='grey', alpha=0.1, lw=0.5, zorder=0, ls='solid')

    ax.add_artist(corrector)
    ax.set_xlim(-1300, 1300)
    ax.set_ylim(-1300, 1300)
    ax.set_aspect('equal', 'box')

    return ax

def PixelPlot(pixel_data : np.ndarray, cmap=plt.cm.viridis, 
              vmin=None, vmax=None, ax=None, norm=None, 
              title=None, markpixels=[], markcolor='red', **kwargs) -> plt.axes :
    """Plot a pixel array to the standard FD display mode of hexagonal grids"""

    from matplotlib.patches import RegularPolygon
    from matplotlib.colors import Normalize

    ax = ax if ax is not None else plt.gca()
    ax.set_title(title if title is not None else '', pad=12)

    vmin = vmin if vmin is not None else np.nanmin(pixel_data)
    vmax = vmax if vmax is not None else np.nanmax(pixel_data)
    norm = norm if norm is not None else Normalize(vmin=vmin, vmax=vmax)

    if isinstance(cmap, str): cmap = plt.get_cmap(cmap)
    
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
            edgecolor=markcolor if ipix in markpixels else 'k',
            lw = kwargs.get('lw', 1),
            zorder=2 if ipix in markpixels else 1
        )

        ax.add_patch(hexagon)

    ax.set_xlim(-15.8, 15.8)
    ax.set_ylim(-15.8, 15.8)
    ax.invert_yaxis()
    ax.axis('off')
    ax.set_aspect(20 / 22)

    return ax