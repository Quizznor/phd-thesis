from ...binaries import uncertainties
from ...binaries import pd
from ...plotting import plt
from . import AperturePlot, PixelPlot

def XYComparisonPlot(*runs : list[dict], cmap=plt.cm.coolwarm, hist_bins=50, vmin=0.6, vmax=1.4, contrast_boost=False) -> plt.figure :
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

        # divide by 50 due to 50 flashes
        pixels /= CalA_signal / 50                                 # normalize pixels to calA pixels
        positions['FDeventSum'] /= np.sum(CalA_signal) / 50        # normalize positions to calA sum

        positions_normalized.append(positions)
        pixels_normalized.append(pixels)

    pixel_ratio = pixels_normalized[0] / pixels_normalized[1]
    positions_ratio = build_xy_ratio(positions_normalized[0], positions_normalized[1])
    mean_positions = uncertainties.ufloat(np.mean(positions_ratio['ratio']), np.std(positions_ratio['ratio']))
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
    ax1.scatter(positions_ratio.x, positions_ratio.y, 
                    c=positions_ratio['ratio'],
                    norm=norm,
                    marker="o", 
                    cmap=cmap, 
                    s = 4)
    ax1.axis('off')
    ax1.set_title('Aperture view')
    ax1.plot([-650, 650], [1350, 1350], c='k', ls='--', clip_on=False)

    # set up camera pixel comparison
    if contrast_boost:
        _min, _max, mean, std = pixel_ratio.min(), pixel_ratio.max(), pixel_ratio.mean(), pixel_ratio.std()
        pixel_plot_ratios = np.interp(pixel_ratio, (mean - 2 * std, mean + 2 * std), (vmin, vmax))
    else:
        pixel_plot_ratios = pixel_ratio

    PixelPlot(pixel_plot_ratios, ax=ax2, cmap=cmap, vmin=vmin, vmax=vmax, norm=norm, title='Camera view' + (' (contrast boost)' if contrast_boost else ''))
    ax2.plot([-7, 7], [-18, -18], c='r', clip_on=False)

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

    ax4.hist(np.clip(positions_ratio['ratio'], vmin, vmax), bins=bins, density=True, histtype='step',
             color='k', ls='--')
    n2, _, patches = ax4.hist(np.clip(positions_ratio['ratio'], vmin, vmax), bins=bins, density=True)
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

def build_xy_ratio(data1, data2):

    pd.options.mode.chained_assignment = None

    # multiple flashes at position 0
    d1_zeros = data1.loc[data1['x'] + data1['y'] == 0]
    d2_zeros = data2.loc[data2['x'] + data2['y'] == 0]

    data1 = pd.concat([data1, d1_zeros, d1_zeros]).drop_duplicates(keep=False)
    data2 = pd.concat([data2, d2_zeros, d2_zeros]).drop_duplicates(keep=False)

    ratios = pd.DataFrame({'x' : 0, 'y' : 0, 'ratio' : d1_zeros['FDeventSum'].mean() / d2_zeros['FDeventSum'].mean()}, index=[0])

    # all other flashes
    smaller_dataset = data1 if len(data1) < len(data2) else data2
    larger_dataset = data2 if len(data1) < len(data2) else data1
    factor = lambda x: x if len(data1) < len(data2) else 1/x

    for i, (_, row) in enumerate(smaller_dataset.iterrows(), 1):
    
        try:

            y_distances = (larger_dataset['y'] - row.loc['y']).abs()
            closest_y = larger_dataset.iloc[y_distances.argmin()]['y']

            same_row = larger_dataset[larger_dataset['y'] == closest_y]
            same_row['x_distances'] = (same_row['x'] - row.loc['x']).abs()

            closest_xy = same_row.iloc[same_row['x_distances'].argmin()]

            closest_distance = np.sqrt( (closest_xy['y'] - row['y'])**2 + (closest_xy['x'] - row['x'])**2 )
            if closest_distance > 10: continue

            ratio = factor(row['FDeventSum']/closest_xy['FDeventSum'])
            this_ratio = pd.DataFrame({'x' : row['x'], 'y' : row['y'], 'ratio' : ratio}, index=[i])
            ratios = pd.concat([ratios, this_ratio], ignore_index=True)
        except ValueError:
            pass

    return ratios

def get_run_numbers(telescope : str, date : str) -> dict : 

    import pickle
    with open('/cr/users/filip/bin/utils/Auger/FD/xy_measurements.pkl', 'rb') as f:
        data = pickle.load(f)

    try:
        requested_telescope = data[telescope]
        for measurement in requested_telescope:
            if measurement['date'] == date:
                return measurement
        else:
            raise KeyError

    except KeyError:
        from ... import create_stream_logger
        import logging

        logger = create_stream_logger('XY-logger', logging.ERROR)
        logger.error(f'requested dataset does not exist! {telescope = }, {date = }')
        return {}