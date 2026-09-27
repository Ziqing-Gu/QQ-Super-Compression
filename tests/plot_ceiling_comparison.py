"""Plot measured, latency-compensated outputs from ceiling_compare.cpp."""
import sys
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

root = Path(sys.argv[1])
plt.rcParams.update({'font.family': 'Microsoft YaHei', 'axes.unicode_minus': False})
fig, axes = plt.subplots(1, 2, figsize=(12, 4.5), constrained_layout=True)
for ax, tp in zip(axes, (0, 1)):
    data = np.genfromtxt(root / f'burst-1000-tp{tp}.csv', delimiter=',', names=True)
    # Quiet 1 kHz carrier after a 20 ms overload. Use a trailing 5 ms RMS
    # window and begin after the entire window has left the overload.
    time = (data['time'] - 1.02) * 1000
    for field, label, color in [('previous', '原固定保持/释放', '#85909b'),
                                ('candidate', '自适应实验版', '#00a5be')]:
        rms = np.sqrt(np.convolve(data[field]**2, np.ones(240)/240, mode='full')[:len(time)])
        db = 20*np.log10(np.maximum(rms, 1e-12) / (.125/np.sqrt(2)))
        keep = (time >= 6) & (time <= 350)
        ax.plot(time[keep], db[keep], label=label, color=color, linewidth=2)
    ax.axhline(-.1, color='#ef7b33', ls='--', lw=1, label='恢复到原音量的 −0.1 dB 以内')
    ax.set(xlim=(0,350), ylim=(-14,.4), xlabel='峰值结束后 / ms',
           ylabel='相对未处理的安静段 / dB', title='TP 开启' if tp else 'SP 模式')
    ax.grid(alpha=.2)
axes[0].legend(loc='lower right', fontsize=9)
fig.suptitle('短峰值后的恢复对比 · 48 kHz / 1 kHz 载波 / Ceiling −1 dB', fontsize=13)
fig.savefig(root / 'ceiling-recovery.png', dpi=160)
print(root / 'ceiling-recovery.png')
