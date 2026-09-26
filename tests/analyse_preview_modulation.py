"""Spectral checks on actual VST3 renders, not a detector simulation.

AM residual is measured against independently known envelope/static-law audio;
it is NOT steady-tone THD. Two-tone non-input energy combines modulation,
harmonics, intermodulation, and any alias products.
"""
from pathlib import Path
import csv
import json
import sys
import numpy as np

root = Path(sys.argv[1])
rows = list(csv.DictReader((root / 'modulation-manifest.csv').open()))
results = []
def power_db(numerator, denominator):
    return float(10*np.log10(max(1e-30, numerator/max(1e-60, denominator))))

for row in rows:
    carrier, mod, tone2 = (int(float(row[k])) for k in ('carrier', 'modulation', 'tone2'))
    result = dict(row)
    if row['kind'] == 'AM':
        ideal = np.fromfile(root / (row['id']+'-ideal.f32'), dtype='<f4')[48000:96000].astype(float)
        denominator = np.sum(abs(np.fft.rfft(ideal))**2)
        bins = np.zeros(24001, dtype=bool)
        width = max(2, mod*3)
        for harmonic in range(2, 8):
            lo, hi = harmonic*carrier-width, harmonic*carrier+width+1
            if 0 <= lo < hi <= len(bins):
                bins[lo:hi] = True
        for version in ('stable', 'preview'):
            y = np.fromfile(root / (row['id']+'-'+version+'.f32'), dtype='<f4')[48000:96000].astype(float)
            error = y-ideal
            result[version+'_harmonic_neighbourhood_residual_dbc'] = power_db(np.sum(abs(np.fft.rfft(error))[bins]**2), denominator)
            result[version+'_tracking_error_db'] = power_db(np.sum(error**2), np.sum(ideal**2))
        result['residual_change_db'] = result['preview_harmonic_neighbourhood_residual_dbc']-result['stable_harmonic_neighbourhood_residual_dbc']
    else:
        for version in ('stable', 'preview'):
            y = np.fromfile(root / (row['id']+'-'+version+'.f32'), dtype='<f4')[48000:96000].astype(float)
            p = abs(np.fft.rfft(y))**2
            mask = np.ones(len(p), dtype=bool)
            mask[[0, carrier, tone2]] = False
            result[version+'_non_input_spectrum_dbc'] = power_db(p[mask].sum(), p[carrier]+p[tone2])
        result['non_input_change_db'] = result['preview_non_input_spectrum_dbc']-result['stable_non_input_spectrum_dbc']
    results.append(result)
fields = list(dict.fromkeys(key for row in results for key in row))
with (root/'modulation-results.csv').open('w', newline='', encoding='utf-8') as f:
    writer = csv.DictWriter(f, fieldnames=fields)
    writer.writeheader()
    writer.writerows(results)
am = [r for r in results if r['kind']=='AM']
tones = [r for r in results if r['kind']=='two-tone']
summary = dict(
    status='MEASURED_PREVIEW_REQUIRES_LISTENING',
    note='AM residual is not steady-tone THD. Positive changes mean more residual energy. No claim of zero distortion on arbitrary signals.',
    case_count=len(results),
    am_worst_regression=max(am, key=lambda r:r['residual_change_db']),
    two_tone_worst_regression=max(tones, key=lambda r:r['non_input_change_db']),
    am_reference_400hz_3hz=[r for r in am if r['carrier']=='400' and r['modulation']=='3'],
    am_stress_50hz_11hz=[r for r in am if r['carrier']=='50' and r['modulation']=='11'],
)
(root/'modulation-summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
print(json.dumps(summary, indent=2))
