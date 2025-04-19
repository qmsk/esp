<style>

</style>

<template>
  <main id="config-view" class="split">
    <div class="view">
      <h1>Configuration</h1>
      <progress v-show="loading">Loading...</progress>
      <template v-if="config">
        <form id="config" method="post" @submit.prevent="submit">
          <fieldset v-for="mod in config.modules" :key="modName(mod)">
            <legend>{{ modName(mod) }}</legend>
            <div class="module-description" v-if="mod.description">
              <p v-for="line in splitlines(mod.description)">{{ line }}</p>
            </div>

            <template v-for="tab in mod.table" v-if="!tab.migrated">
              <label :for="mod.name + '-' + tab.name">{{ tab.name }}</label>
              <div class="tab-values">
                <template v-for="value in fieldValues(mod, tab)">
                  <input v-if="tab.type == 'uint16'" type="number"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                    :value="value" min="0" :max="tab.uint16_max ? tab.uint16_max : 65536"
                    :readonly="tab.readonly">

                  <input v-if="tab.type == 'string' && !tab.secret" type="text"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                    :value="value"
                    :readonly="tab.readonly">

                  <input v-if="tab.type == 'string' && tab.secret" type="password"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                    :value="value"
                    :readonly="tab.readonly">

                  <input v-if="tab.type == 'bool'" type="checkbox"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                    :value="true" :checked=value
                    :readonly="tab.readonly">

                  <select v-if="tab.type == 'enum'"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                    :disabled="tab.readonly">
                    <option v-for="v in tab.enum_values" :value="v" :selected="value == v">{{ v }}</option>
                  </select>

                  <select v-if="tab.type == 'file'"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :title="tab.description"
                    :disabled="tab.readonly">
                    <option value="" :selected="!value"></option>
                    <option v-for="v in tab.file_values" :value="v" :selected="value == v">{{ v }}</option>
                  </select>
                </template>

                <div class="errors" v-if="configErrors && configErrors.set && configErrors.set[fieldName(mod, tab)]">
                  {{ configErrors.set[fieldName(mod, tab)] }}
                </div>
              </div>
            </template>
          </fieldset>
        </form>
      </template>
    </div>
    <div class="controls">
      <h2>Apply</h2>
      <fieldset class="actions">
        <button type="submit" form="config">Apply</button>
        <progress class="input" v-show="applying">Applying...</progress>
        <div class="symbol error" v-if="applyError" :title="applyError">!</div>
      </fieldset>

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

    // 
    applying: false,
    applyError: null,

    // restore
    uploading: false,
    restoreError: null,
  }),
  created() {
    this.load();
  },
  computed: {
    config() {
      return this.$store.state.config;
    },
    configErrors() {
      return this.$store.state.configErrors;
    },
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
    modName(mod) {
      if (mod.index) {
        return mod.name + mod.index.toString();
      } else {
        return mod.name;
      }
    },
    fieldName(mod, tab) {
      if (mod.index) {
        return '[' + mod.name + mod.index.toString() + ']' + tab.name;
      } else {
        return '[' + mod.name + ']' + tab.name;
      }
    },
    fieldValues(mod, tab) {
      if (tab.count !== undefined) {
        return  [...Array(tab.size).keys()].map(i => {
          return i < tab.count ? tab.values[tab.type][i] : null;
        });
      } else {
        return [tab.value[tab.type]];
      }
    },
    restoreInvalid(event) {
      const input = event.target;

      this.restoreError = input.validity ? null : input.validationMessage;
    },
    async submit(event) {
      const form = event.target;
      const formdata = new FormData(form);

      this.applying = true;

      try {
        await this.$store.dispatch('postConfig', formdata);
        await this.$store.dispatch('restartSystem');
        await this.$store.dispatch('loadConfig');
      } catch (error) {
        if (error.name == "APIError" && error.data) {
          this.applyError = error.message;
        } else {
          throw error;
        }
      } finally {
        this.applying = false;
      }
    },
    async restoreSubmit(event) {
      const form = event.target;
      const input = form.elements["file"];
      const file = input.files[0];

      this.uploading = true;

      try {
        await this.$store.dispatch('uploadConfig', file);
        await this.$store.dispatch('restartSystem');
        await this.$store.dispatch('loadConfig');
      } catch (error) {
        input.setCustomValidity(error.name + ": " + error.message);
      } finally {
        this.uploading = false;
      }
    },
  }
}
</script>
