const map = new Map([
    ['d', 24 * 60 * 60 ],
    ['h', 60 * 60 ],
    ['m', 60 ],
    ['s', 1 ],
    ['ms', 1 / 1000 ],
  ]);
  
  export default function (value, unit = null, round = null) {
    let parts = [];

    if (unit) {
      value = value * map.get(unit);
    }
    if (round) {
      round = map.get(round);
    }

    for (const [suffix, unit] of map) {
      if (unit < round) {
        break;
      }

      if (value >= unit) {
        const units = value / unit;
        value = value % unit;

        parts.push(units.toFixed(0) + suffix);
      }
    }

    if (parts.length > 0) {
      return parts.join('');
    } else {
      return value + 's';
    }
  }
  