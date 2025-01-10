from Monit import Monit
import matplotlib.pyplot as plt

test = Monit(2024, 12, 24)

test.keys()

time = test('fTime', 136)
peak = test('fPeak[4]', 136)

for i in range(4):
    plt.plot(time, peak[:, i], label=i+1)

plt.legend()
#plt.show()
plt.savefig('test.png')
