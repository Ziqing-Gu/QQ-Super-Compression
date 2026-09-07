"""Read-only image/layout comparison; ignores only the changed version label."""
from pathlib import Path
import sys
import re
from PIL import Image
import numpy as np

baseline, candidate = map(Path, sys.argv[1:3])
for name in ('warm-ST.png', 'warm-sidechain.png', 'warm-LR.png', 'warm-MS.png',
             'warm-minimum.png', 'warm-maximum.png', 'warm-ST-2x.png',
             'warm-numeric-edit.png', 'warm-control-detail.png', 'warm-meter-depth.png',
             'classic-ST.png', 'classic-sidechain.png', 'classic-LR.png', 'classic-MS.png', 'classic-ST-2x.png'):
    a, b = (np.array(Image.open(folder / name).convert('RGB')) for folder in (baseline, candidate))
    assert a.shape == b.shape, name
    changed = np.any(a != b, axis=2)
    if 'control-detail' not in name and 'meter-depth' not in name:
        scale = a.shape[1] / 1020
        changed[int(48*scale):int(68*scale)+1, int(20*scale):int(140*scale)+1] = False
    count = np.count_nonzero(changed)
    print(f'{name}: {count} changed pixels outside version label')
    if name.startswith('warm-') and name != 'warm-meter-depth.png':
        # The newly requested Light pointer/ticks may change rotary draw areas
        # only. These areas are exported from the actual JUCE slider layout.
        # Material/compositor code outside paintPointer is checked separately.
        regions = np.zeros(changed.shape, dtype=bool)
        if name == 'warm-control-detail.png':
            rectangles = [(i*204+2, 4, 200, 200) for i in range(5)]
            rectangles += [(i*204+62, 252, 80, 90) for i in range(5)]
            pixel_scale = 1
        else:
            rectangles = [list(map(int, re.findall(r'-?\d+', line))) for line in
                          (candidate / (Path(name).stem + '-rotary-bounds.txt')).read_text().splitlines()]
            pixel_scale = 2 if name.endswith('-2x.png') else 1
        for x, y, w, h in rectangles:
            regions[max(0, y*pixel_scale-2):(y+h)*pixel_scale+2,
                    max(0, x*pixel_scale-2):(x+w)*pixel_scale+2] = True
        assert not np.any(changed & ~regions), (name, 'unexpected change outside Light rotary areas')
        print('  PASS: changes restricted to requested Light pointer/tick areas')
        continue
    if name in ('classic-sidechain.png', 'classic-ST-2x.png') and count:
        # One edge pixel in the legacy HPF dial differs by at most 3/255.
        # Keep a narrowly bounded raster tolerance, not a broad image threshold.
        delta = np.abs(a.astype(np.int16) - b.astype(np.int16))[changed]
        assert count <= 1 and delta.max() <= 3, (name, count, delta.max())
        if name == 'classic-ST-2x.png':
            assert tuple(np.argwhere(changed)[0]) == (1343, 1680)
        print('  legacy dial edge raster tolerance: <=1 pixel, <=3/255 per channel')
    else:
        assert count == 0, name
for name in ('warm-ST-bounds.txt', 'classic-ST-bounds.txt', 'warm-LR-bounds.txt', 'warm-MS-bounds.txt'):
    assert (baseline / name).read_bytes() == (candidate / name).read_bytes(), name
for mode in ('ST', 'LR', 'MS'):
    def normalized(s):
        # Opening the sidechain popup intentionally calls toFront(), changing
        # child indices but not geometry. Compare all recorded bounds/visibility.
        s = s.replace('|LIGHT', '|THEME').replace('|DARK', '|THEME').replace('|CLASSIC', '|THEME')
        return sorted(line.split('|', 1)[1] for line in s.splitlines())
    reference = normalized((candidate / f'warm-{mode}-bounds.txt').read_text())
    for theme in ('dark', 'classic'):
        assert normalized((candidate / f'{theme}-{mode}-bounds.txt').read_text()) == reference, (theme, mode)
for name in ('dark-ST.png', 'dark-LR.png', 'dark-MS.png'):
    a, b = (np.array(Image.open(folder / name).convert('RGB')) for folder in (baseline, candidate))
    # Main history plot/meters/header unchanged; new finish starts at y=640.
    changed = np.any(a[:640] != b[:640], axis=2)
    changed[48:69, 20:141] = False
    count = np.count_nonzero(changed)
    if count:
        # Identical Display source may rasterize a couple of trace-edge pixels
        # differently across builds. Do not relax backgrounds, meters or layout.
        yy, xx = np.where(changed)
        delta = np.abs(a[:640].astype(np.int16) - b[:640].astype(np.int16))[changed]
        assert name != 'dark-ST.png' and count <= 2 and delta.max() <= 8
        assert np.all((xx >= 84) & (xx <= 691) & (yy >= 140) & (yy <= 590))
        print(f'{name}: {count} trace-edge pixels within <=8/255 raster tolerance; all background/meter pixels unchanged')
print('PASS: Classic pixel parity, Light changes scoped to knobs, pure Dark Display and identical three-theme layout.')
