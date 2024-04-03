from ..plotting import plt
from ..binaries import np
from ..binaries import pd
from ..binaries import uncertainties

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
            lw = kwargs.get('lw', 1)
        )

        ax.add_patch(hexagon)

    ax.set_xlim(-15.8, 15.8)
    ax.set_ylim(-15.8, 15.8)
    ax.invert_yaxis()
    ax.axis('off')
    ax.set_aspect(20 / 22)

    return ax

def XYComparisonPlot(*runs : list[dict], cmap=plt.cm.coolwarm, hist_bins=50, vmin=0.6, vmax=1.4) -> plt.figure :
    """Compare the results of two XY runs, normalized to their respective Cal A signal"""

    from matplotlib.colors import Normalize
    from matplotlib.gridspec import GridSpec
    from matplotlib.colorbar import ColorbarBase

    assert len(runs) == 2, "please only compare two runs at a time"

    fig = plt.figure()
    gs = GridSpec(
        2,
        4,
        figure=fig,
        width_ratios=[1, 1, 0.8, 0.05],
        height_ratios = [1, 1],
    )
    gs.update(left=0.05, right=0.95, wspace=0.05, hspace=0.02)

    positions_normalized, pixels_normalized = [], []
    result_dir = '/cr/data01/filip/xy-calibration/results'
    for run in runs:
        xy, CalAs = run['XY'], run['CalA_open_shutter']
        try:
            CalA_signal = np.zeros(440)
            for CalA in CalAs:
                CalA_signal += np.loadtxt(f"{result_dir}/out_{CalA}.txt", usecols=[2])

        except (IndexError, AssertionError):
            print("Malformed CalA data received, please make sure you pass \
                in two CalAs (pre/post-XY), which have 440 pixels of data")
            return fig

        pixels = np.loadtxt(f"{result_dir}/out_{xy}.txt", usecols=[2])
        positions = pd.read_csv(f"{result_dir}/outPositionsComb_{xy}.txt", usecols=['x', 'y', 'FDeventSum'])

        pixels /= CalA_signal                                   # normalize pixels to calA pixels
        positions['FDeventSum'] /= np.sum(CalA_signal)          # normalize positions to calA sum

        positions_normalized.append(positions)
        pixels_normalized.append(pixels)

    pixel_ratio = pixels_normalized[0] / pixels_normalized[1]
    positions_combined = positions_normalized[0].merge(positions_normalized[1] ,on =['x','y']).drop_duplicates().dropna()
    positions_ratio = positions_combined.FDeventSum_x / positions_combined.FDeventSum_y
    mean_positions = uncertainties.ufloat(np.mean(positions_ratio), np.std(positions_ratio))
    mean_pixels = uncertainties.ufloat(np.mean(pixel_ratio), np.std(pixel_ratio))

    ax1 = fig.add_subplot(gs[:, 0])
    ax2 = fig.add_subplot(gs[:, 1])
    ax3 = fig.add_subplot(gs[0, 2])
    ax4 = fig.add_subplot(gs[1, 2], sharex=ax3)
    ax5 = fig.add_subplot(gs[:, 3])

    # set up colorbar for position and pixel comparison
    norm = Normalize(vmin=vmin, vmax=vmax)
    date1, date2 = runs[0]['date'], runs[1]['date']
    ColorbarBase(ax5, cmap=cmap, norm=norm, orientation='vertical', 
                 label=fr"$\tilde{{\mathrm{{XY}}}}_\mathrm{{{date1}}}\,/\,\tilde{{\mathrm{{XY}}}}_\mathrm{{{date2}}}$")

    # set up aperture position comparison
    AperturePlot(ax1)
    c0 = ax1.scatter(positions_combined.x, positions_combined.y, c=positions_ratio,
                    norm=norm,
                    marker="o", cmap=cmap, s = 4)
    ax1.axis('off')
    ax1.set_title('Aperture view')

    # set up camera pixel comparison
    _min, _max, mean, std = pixel_ratio.min(), pixel_ratio.max(), pixel_ratio.mean(), pixel_ratio.std()
    pixel_contrast_boost = np.interp(pixel_ratio, (mean - 2 * std, mean + 2 * std), (vmin, vmax))
    PixelPlot(pixel_contrast_boost, ax=ax2, cmap=cmap, vmin=vmin, vmax=vmax, norm=norm, title='Camera view (contrast boost)')

    # histograms for comparison
    bins = np.linspace(vmin, vmax, hist_bins)
    bin_centers = 0.5 * (bins[1:] + bins[:-1])
    ax3.hist(np.clip(pixel_ratio, vmin, vmax), bins=bins, density=True, histtype='step',
             color='r', ls='solid')
    n1, _, patches = ax3.hist(np.clip(pixel_ratio, vmin, vmax), bins=bins, density=True)
    ax3.text(0.02, 0.98,
             fr"$\langle\tilde{{\mathrm{{XY}}}}^\mathrm{{pix.}}_\mathrm{{{date1}}}\,/\,\tilde{{\mathrm{{XY}}}}^\mathrm{{pix.}}_\mathrm{{{date2}}}\rangle = {mean_pixels.format('S')}$",
             horizontalalignment='left',
             verticalalignment='top',
             transform=ax3.transAxes,
             fontdict={'fontsize' : 7},
             )

    for x, p in zip(bin_centers, patches):
        plt.setp(p, 'facecolor', cmap(norm(x)))

    ax4.hist(np.clip(positions_ratio, vmin, vmax), bins=bins, density=True, histtype='step',
             color='k', ls='--')
    n2, _, patches = ax4.hist(np.clip(positions_ratio, vmin, vmax), bins=bins, density=True)
    ax4.text(0.02, 0.98,
            fr"$\langle\tilde{{\mathrm{{XY}}}}^\mathrm{{pos.}}_\mathrm{{{date1}}}\,/\,\tilde{{\mathrm{{XY}}}}^\mathrm{{pos.}}_\mathrm{{{date2}}}\rangle = {mean_positions.format('S')}$",
            horizontalalignment='left',
            verticalalignment='top',
            transform=ax4.transAxes,
            fontdict={'fontsize' : 7},
            )
    for x, p in zip(bin_centers, patches):
        plt.setp(p, 'facecolor', cmap(norm(x)))

    

    ax3.set_ylim(1e-2, 1.2 * max(n1))
    ax4.set_ylim(1e-2, 1.2 * max(n2))  
    # ax3.legend(fontsize=7, loc='upper left')
    # ax4.legend(fontsize=7, loc='upper left')
    ax3.set_yticks([])
    ax4.set_yticks([])

    return fig