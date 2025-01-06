<style>
</style>
<template>
  <button @click="click">

  </button>
</template>
<script>
export default {
  props: [
    'pressAction',
  ],
  data: () => ({
    active: false,
  }),
  methods: {
    async click() {
      this.active = true;

      try {
        await this.$store.dispatch(this.pressAction);
      } finally {
        this.active = false;
      }
    }
  }
}
</script>
