<style>

</style>

<template>
  <main id="files-view" class="split">
    <div class="view">
      <h1>Files</h1>
      <progress v-show="loading">Loading...</progress>
    </div>
  </main>
</template>
<script>
export default {
  data: () => ({
    loading: true,
  }),
  created() {
    this.load();
  },
  computed: {
  },
  methods: {
    async load() {
      this.loading = true;
    }
  }
}
</script>
