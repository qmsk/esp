<style>

</style>
<template>
  <span>{{ timerMetric.util | percentage }} @ {{ timerMetric.rate | rate }} <sub>@ {{ timerMetric.interval | interval }}</sub></span>
</template>
<script>
export default {
  props: {
    timerMetric: Object,
  },
}
</script>
