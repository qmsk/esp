const sizeMap = new Map([
  ['GB', 1024 * 1024 * 1024 ],
  ['MB', 1024 * 1024 ],
  ['KB', 1024 ],
]);

export default function (value) {
  for (const [suffix, unit] of sizeMap) {
    if (value >= unit) {
      const units = value / unit;

      return units.toFixed(2) + ' ' + suffix;
    }
  }

  return value.toFixed(0) +  ' ' + 'B';
}
