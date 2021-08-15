<style>

</style>

<template>
  <main id="config-view" class="split">
    <div class="view">
      <h1>Configuration</h1>
      <progress v-show="loading">Loading...</progress>
      <template v-if="config">
        <form action="/api/config" method="post">
          <fieldset v-for="mod in config.modules" :key="mod.name">
            <legend>{{ mod.name }}</legend>
            <div class="module-description" v-if="mod.description">
              <p v-for="line in splitlines(mod.description)">{{ line }}</p>
            </div>

            <template v-for="tab in mod.table">
              <label :for="mod.name + '-' + tab.name">{{ tab.name }}</label>
              <input v-if="tab.type == 'uint16'" type="number"
                :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                :value="tab.value.uint16" min="0" :max="tab.max ? tab.max : 65536"
                :readonly="tab.readonly">
              <input v-if="tab.type == 'string' && !tab.secret" type="text"
                :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                :value="tab.value.string"
                :readonly="tab.readonly">
              <input v-if="tab.type == 'string' && tab.secret" type="password"
                :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                :value="tab.value.string"
                :readonly="tab.readonly">
              <input v-if="tab.type == 'bool'" type="hidden"
                :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                :value="false"
                >
              <input v-if="tab.type == 'bool'" type="checkbox"
                :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                :value="true" :checked=tab.value.bool
                :readonly="tab.readonly">

              <select v-if="tab.type == 'enum'"
                :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                :disabled="tab.readonly">
                <option v-for="value in tab.enum_values" :value="value">{{ value }}</option>
              </select>
            </template>
          </fieldset>

          <fieldset class="actions">
            <button type="submit">Apply</button>
          </fieldset>
        </form>
      </template>
    </div>
    <div class="controls">
      <h2>Backup</h2>
      <form method="get" action="/config.ini">
        <fieldset>
          <button type="submit">Backup</button>
        </fieldset>
      </form>

      <h2>Restore</h2>
      <form method="post" @submit.prevent="restoreSubmit">
        <fieldset>
          <label for="restore-file">File</label>
          <input type="file" id="restore-file" name="file" @invalid="restoreInvalid"
            accept="text/plain,.ini">
          <progress class="input" v-show="uploading">Uploading...</progress>

          <button type="submit" :disabled="uploading">Restore</button>
        </fieldset>
      </form>
    </div>
  </main>
</template>

<script>
export default {
  data: () => ({
    loading: true,

    // restore
    uploading: false,
    restoreError: null,
  }),
  created() {
    this.load()
  },
  computed: {
    config() {
      return this.$store.state.config
    }
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadConfig')
      } finally {
        this.loading = false;
      }
    },
    splitlines(description) {
      return description.split('\n');
    },
    fieldName(mod, tab) {
      return '[' + mod.name + ']' + tab.name;
    },
    restoreInvalid(event) {
      const input = event.target;

      this.restoreError = input.validity ? null : input.validationMessage;
    },
    async restoreSubmit(event) {
      const form = event.target;
      const input = form.elements["file"];
      const file = input.files[0];

      this.uploading = true;

      try {
        await this.$store.dispatch('uploadConfig', file);
      } catch (error) {
        input.setCustomValidity(error.name + ": " + error.message);
      } finally {
        this.uploading = false;
      }
    },
  }
}
</script>
