import rateFilter from "./rate.filter"
import percentageFilter from "./percentage.filter"
import intervalFilter from "./interval.filter";

export default function(value) {
  return percentageFilter(value.util) + ' @ ' + rateFilter(value.rate) + ' @ ' + intervalFilter(value.interval);
}
