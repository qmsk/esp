import rateFilter from "./rate.filter"
import percentageFilter from "./percentage.filter"
import intervalFilter from "./interval.filter";

export default function(value) {
  return rateFilter(value.rate) + ' @ ' + percentageFilter(value.util) + ' / ' + intervalFilter(value.interval);
}
