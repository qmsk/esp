import rateFilter from "./rate.filter"
import intervalFilter from "./interval.filter";

export default function(value) {
  return rateFilter(value.rate) + ' / ' + intervalFilter(value.interval);
}
