<style>

</style>
<template>
  <span>{{ counterMetric.rate | rate }} <sub>@ {{ counterMetric.interval | interval }}</sub></span>
</template>
<script>
export default {
  props: {
    counterMetric: Object,
  },
}
</script>
