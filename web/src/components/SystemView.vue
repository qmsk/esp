<style>

</style>

<template>
  <main id="config-view" class="split">
    <div class="view">
      <h1>System</h1>
      <progress v-show="restarting">Restarting...</progress>

    </div>
    <div class="controls">
      <form @submit="restartSubmit">
        <fieldset>
          <button type="submit">Restart</button>
        </fieldset>
      </form>
    </div>
  </main>
</template>

<script>
export default {
  data: () => ({
    restarting: false,
  }),
  methods: {
    async restartSubmit(event) {
      this.restarting = true;

      try {
        await this.$store.dispatch('restartSystem');
      } catch (error) {

      } finally {
        this.restarting = false;
      }
    }
  }
}
</script>
