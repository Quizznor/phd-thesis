import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import scienceplots

matplotlib.use('TkAgg')

plt.style.use(['science', 'ieee', 'no-latex'])
plt.rcParams['figure.figsize'] = [3.3, 3.3]
fig, ax = plt.subplots(figsize=(1.8, 1.8))

muon = 0.1 * np.random.lognormal(3, 1, 2000)
electronics = np.random.uniform(-30, 30, 100)

n_mu, _ = np.histogram(muon, bins=100, range=[-60, 100])
signal = n_mu + electronics

ax.axhline(0, c='k', ls='--', lw=0.2, zorder=0)
ax.bar(range(len(n_mu)), signal, width=1, color='gray')

start, stop = 37, 49
ax.bar(range(start, stop), signal[start:stop], width=1, alpha=0.8, color='r')
ax.plot([i//2-0.5 for i in range(2*len(n_mu))], [signal[i//2-1] for i in range(1, 1+2*len(n_mu))],
         ls='-', c='k', lw=0.3)

offset = 0.3
half_sum = offset * np.sum(signal)
start_sum = np.sum(signal[:start-1])

ax.plot(range(len(n_mu)), np.cumsum(signal) - half_sum, lw=0.5, color='gray', label='Background', ls='--', zorder=0)
#ax.plot(range(start-1, stop), start_sum - half_sum + np.cumsum(signal[start-1:stop]), 
#        zorder=0, lw=0.9, color='white', ls='-')
ax.plot(range(start-1, stop), start_sum - half_sum + np.cumsum(signal[start-1:stop]), 
         lw=0.5, ls='-', c='r', label='Muon signal', zorder=0)

ax.text(100, np.sum(signal) * (0.95 - offset), '$Q_\mathrm{MIP}$',
         ha='right', va='top', fontsize='x-small')
ax.text(100, np.max(signal) * 0.95, '$I_\mathrm{MIP}$',
         ha='right', va='top', fontsize='x-small')
ax.axhline(np.max(signal) * 0.99, xmin=0.397, ls='--', lw=0.5, color='gray', zorder=0)

ax.set_xticks([], [])
ax.set_yticks([], [])
ax.set_xlabel('time / arb. units')
ax.set_ylabel('SSD response / arb. units')
ax.legend(fontsize='xx-small')
#plt.show()

plt.tight_layout()
fig.savefig('peak-charge.png')
