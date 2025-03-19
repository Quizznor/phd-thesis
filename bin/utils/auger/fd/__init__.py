from ...plotting import plt
from ...binaries import np

from matplotlib.colors import Normalize
from matplotlib.gridspec import GridSpec
from matplotlib.cm import ScalarMappable


def AperturePlot(ax=None, filterStructure=True) -> plt.axes:
    """Add aperture, corrector, lens structure of FD telescopes to a given axis"""

    ax = ax if not ax is None else plt.gca()

    aperture = plt.Circle((0, 0), 1100, color="tab:red", fill=False, lw=2.5, zorder=1)
    ax.add_artist(aperture)
    corrector = plt.Circle((0, 0), 1700 / 2, color="k", fill=False, ls="--", zorder=1)

    if filterStructure:
        ax.plot(
            [-1030, 1030],
            [765 / 2] * 2,
            color="grey",
            alpha=0.2,
            lw=0.8,
            zorder=0,
            ls="solid",
        )
        ax.plot(
            [-1030, 1030],
            [-765 / 2] * 2,
            color="grey",
            alpha=0.2,
            lw=0.8,
            zorder=0,
            ls="solid",
        )

        ax.plot(
            [-450 / 2] * 2,
            [-1078, 1076],
            color="grey",
            alpha=0.2,
            lw=0.8,
            zorder=0,
            ls="solid",
        )
        ax.plot(
            [450 / 2] * 2,
            [-1078, 1076],
            color="grey",
            alpha=0.2,
            lw=0.8,
            zorder=0,
            ls="solid",
        )
        ax.plot(
            [450 / 2 + 460] * 2,
            [-850, 850],
            color="grey",
            alpha=0.1,
            lw=0.5,
            zorder=0,
            ls="solid",
        )
        ax.plot(
            [-450 / 2 - 460] * 2,
            [-850, 850],
            color="grey",
            alpha=0.1,
            lw=0.5,
            zorder=0,
            ls="solid",
        )

    ax.add_artist(corrector)
    ax.set_xlim(-1300, 1300)
    ax.set_ylim(-1300, 1300)
    ax.set_aspect("equal", "box")

    return ax


def PixelPlot(
    pixel_data: np.ndarray,
    ax=None,
    title=None,
    vmin=None,
    vmax=None,
    norm=None,
    cmap=None,
    markpixels=[],
    markcolor="red",
    **kwargs,
) -> plt.axes:
    """Plot a pixel array to the standard FD display mode of hexagonal grids"""

    from matplotlib.patches import RegularPolygon
    from matplotlib.colors import Normalize

    ax = ax if ax is not None else plt.gca()
    ax.set_title(title if title is not None else ax.get_title())
    three_sigma_cut = lambda d: d[np.abs(d - np.nanmean(d))/np.nanstd(d) < 3]

    cmap = cmap if cmap is not None else plt.cm.viridis
    vmin = vmin if vmin is not None else np.min(three_sigma_cut(pixel_data))
    vmax = vmax if vmax is not None else np.max(three_sigma_cut(pixel_data))
    norm = norm if norm is not None else Normalize(vmin=vmin, vmax=vmax)

    if isinstance(cmap, str):
        cmap = plt.get_cmap(cmap)

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
            numVertices=6,
            radius=0.866,
            orientation=np.radians(60),
            facecolor=cmap(norm(pixel)),
            edgecolor=markcolor if ipix in markpixels else "k",
            lw=kwargs.get("lw", 1),
            zorder=2 if ipix in markpixels else 1,
        )

        ax.add_patch(hexagon)

    ax.set_xlim(-15.8, 15.8)
    ax.set_ylim(-15.8, 15.8)
    ax.invert_yaxis()
    ax.set_aspect(20 / 22)

    if kwargs.get("axis_off", True):
        ax.axis("off")
    return ax


def get_mirror_or_telescope(mirror_or_telescope: str) -> str:

    if mirror_or_telescope.lower().startswith("m"):
        n_mirror = int(mirror_or_telescope[1:])
        mirror_number = n_mirror % 6 if n_mirror % 6 else 6

        if n_mirror > 24:
            mirror_or_telescope = f"he{mirror_number}"
        elif n_mirror > 18:
            mirror_or_telescope = f"co{mirror_number}"
        elif n_mirror > 12:
            mirror_or_telescope = f"la{mirror_number}"
        elif n_mirror > 6:
            mirror_or_telescope = f"lm{mirror_number}"
        else:
            mirror_or_telescope = f"ll{mirror_number}"
    else:
        n_mirror = int(mirror_or_telescope[2:])

        if mirror_or_telescope.lower().startswith("he"):
            n_mirror += 24
        elif mirror_or_telescope.lower().startswith("co"):
            n_mirror += 18
        elif mirror_or_telescope.lower().startswith("la"):
            n_mirror += 12
        elif mirror_or_telescope.lower().startswith("lm"):
            n_mirror += 6

        mirror_or_telescope = f"m{n_mirror}"

    return mirror_or_telescope


def pixel_grid(data: np.ndarray, **kwargs) -> plt.Figure:

    cols, rows, npix = data.shape

    three_sigma_cut = lambda d: d[np.abs(d - np.nanmean(d))/np.nanstd(d) < 3]
    vmin, vmax = kwargs.get("vmin", None), kwargs.get("vmax", None)
    match vmin:
        case "min": vmin = np.min(three_sigma_cut(data))
        case None: vmin = None
        case _: pass
    match vmax:
        case "max": vmax = np.max(three_sigma_cut(data))
        case None: vmax = None
        case _: pass
        
    cmap = plt.get_cmap(kwargs.get("cmap", "viridis"))
    cbar_label = kwargs.get("cbar_label", "data")
    ylabel = kwargs.get("ylabel", [""] * rows)
    xlabel = kwargs.get("xlabel", [""] * cols)


    fig = plt.figure()
    gs = GridSpec(rows, cols + 1, fig,
                  hspace=0.01, wspace=0.01,
                  width_ratios=[1 for _ in range(cols)] + [0.02 * cols])

    for i in range(rows):
        for j in range(cols):

            ax = fig.add_subplot(gs[i, j])
            if sum(np.isnan(data[j, i])) == 440:
                ax.set_aspect("equal")
                ax.axis("off")
            else:
                PixelPlot(data[j, i], ax, cmap=cmap, vmin=vmin, vmax=vmax, lw=0.4)
            
            if not j: 
                ax.text(-0.1, 0.5,
                        ylabel[i],
                        horizontalalignment='center',
                        verticalalignment='center',
                        transform=ax.transAxes,
                        rotation=90)
            if not i: 
                ax.text(0.5, 1.1,
                    xlabel[j],
                    horizontalalignment='center',
                    verticalalignment='center',
                    transform=ax.transAxes)

    if vmin is not None or vmax is not None:
        cax = fig.add_subplot(gs[:, -1])
        mappable = ScalarMappable(Normalize(vmin, vmax, cmap.N), cmap)
        plt.colorbar(mappable, cax=cax, label=cbar_label)

    return fig