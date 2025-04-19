<style>
  div.config-state {
    float: right;

    margin: 1em auto;
    padding: 1em;
    width: 50%;
    border-radius: 1em;

    text-align: center;
  }

  div.config-state-init {
    background-color: #bbbbbb88;
    color: #111;

  }

  div.config-state-load {
    background-color: #44aaff88;
    color: #ccc;
  }

  div.config-state-boot {
    background-color: #44884488;
    color: #111;
  }

  div.config-state-dirty {
    background-color: #ff880088;
    color: #111;
  }

  div.config-state-invalid {
    background-color: #aaaa2288;
    color: #111;
  }

  div.config-state-save {
    background-color: #44bb4488;
    color: #111;
  }

  div.config-state-reset {
    background-color: #ffbb0088;
    color: #111;
  }

  div.config-state-error {
    background-color: #ff444488;
    color: #111;
  }

  span.state {
    font-family: monospace;
    font-size: large;
    font-weight: bold;
  }

  span.interval {
    font-style: italic;
  }

  span.filename {
    font-family: monospace;
  }

</style>

<template>
  <main id="config-view" class="split">
    <div class="view">
      <h1>Configuration</h1>
      <progress v-show="loading">Loading...</progress>

      <template v-if="config">
        <form id="config" method="post" @submit.prevent="submit">
          <fieldset v-for="mod in config.modules" :key="modName(mod)" :class="{ collapse: true, open: showModule(mod), close: !showModule(mod) }">
            <legend @click="toggleModule(mod)">
              <span>{{ modName(mod) }}</span>

              <button type="button">
                <template v-if="showModule(mod)">
                  -
                </template>
                <template v-else>
                  +
                </template>
              </button>
            </legend>
            <div class="module-description" v-if="mod.description">
              <p v-for="line in splitlines(mod.description)">{{ line }}</p>
            </div>

            <template v-for="tab in mod.table" v-if="!tab.migrated">
              <label :for="mod.name + '-' + tab.name">{{ tab.name }}</label>
              <div class="inputs">
                <template v-for="(value, index) in fieldValues(mod, tab)">
                  <input v-if="tab.type == 'uint16'" type="number"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :data-index="index"
                    :value="value" min="0" :max="tab.uint16_max ? tab.uint16_max : 65536"
                    @input="syncInput"
                    :class="{ invalid: fieldErrors(mod, tab) }"
                    :readonly="tab.readonly"
                  >

                  <input v-if="tab.type == 'string' && !tab.secret" type="text"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :data-index="index"
                    :value="value"
                    @input="syncInput"
                    :class="{ invalid: fieldErrors(mod, tab) }"
                    :readonly="tab.readonly"
                  >

                  <input v-if="tab.type == 'string' && tab.secret" type="password"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :data-index="index"
                    :value="value"
                    @input="syncInput"
                    :class="{ invalid: fieldErrors(mod, tab) }"
                    :readonly="tab.readonly"
                  >

                  <input v-if="tab.type == 'bool'" type="checkbox"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :data-index="index"
                    :value="true" :checked=value
                    @input="syncInput"
                    :class="{ invalid: fieldErrors(mod, tab) }"
                    :readonly="tab.readonly"
                  >

                  <select v-if="tab.type == 'enum'"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :data-index="index"
                    @input="syncInput"
                    :class="{ invalid: fieldErrors(mod, tab) }"
                    :disabled="tab.readonly"
                  >
                    <option v-for="v in tab.enum_values" :value="v" :selected="value == v">{{ v }}</option>
                  </select>

                  <select v-if="tab.type == 'file'"
                    :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)" :data-index="index"
                    @input="syncInput"
                    :class="{ invalid: fieldErrors(mod, tab) }"
                    :disabled="tab.readonly"
                  >
                    <option value="" :selected="!value"></option>
                    <option v-for="v in tab.file_values" :value="v" :selected="value == v">{{ v }}</option>
                  </select>
                </template>
              </div>

              <div class="actions">

              </div>

              <div class="errors" v-for="error in fieldErrors(mod, tab)">
                {{ error }}
              </div>

              <div class="description">
                <p v-for="line in splitlines(tab.description)">{{ line }}</p>
              </div>
            </template>
          </fieldset>
        </form>
      </template>
    </div>
    <div class="controls">
      <h2>State</h2>
      <progress v-if="loading">Loading...</progress>

      <div :class="['config-state', configStateClass]" v-if="configState">
          <span class="state">{{ configState.state }}</span>
        @ <span class="interval">{{ configState.tick_ms | interval('ms', 's')  }}</span>
        (<span class="filename">{{ configState.filename }}</span>)
      </div>

      <h2>Apply</h2>
      <fieldset class="actions">
        <button type="submit" form="config">Apply</button>
        <progress v-show="applying">Applying...</progress>
        <div class="symbol error" v-if="applyError" :title="applyError">!</div>
      </fieldset>

      <h2>Backup</h2>
      <form method="get" action="/config.ini">
        <fieldset class="actions">
          <button type="submit">Backup</button>
        </fieldset>
      </form>

      <h2>Restore</h2>
      <form method="post" @submit.prevent="restoreSubmit">
        <fieldset class="actions">
          <label for="restore-file">File</label>
          <input type="file" id="restore-file" name="file" @invalid="restoreInvalid"
            accept="text/plain,.ini">
          <progress v-show="uploading">Uploading...</progress>

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
    configValues: null,
    configErrors: null,

    // 
    showModules: {},

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
    configState() {
      return this.$store.state.configState;
    },
    configStateClass() {
      if (this.configState) {
        return 'config-state-' + this.configState.state.toLowerCase();
      } else {
        return null;
      }
    },
  },
  methods: {
    updateConfigValues(config) {
      let configValues = {};

      for (const mod of config.modules) {
        for (const tab of mod.table) {
          let name = this.fieldName(mod, tab);

          if (tab.migrated) {
            continue;
          }

          if (tab.count === undefined) {
            configValues[name] = [tab.value[tab.type]];
          } else {
            configValues[name] = [...Array(tab.size).keys()].map(i => i < tab.count ? tab.values[tab.type][i] : null);
          }
        }
      }

      this.configValues = configValues;
    },
    syncInput(event) {
      let name = event.target.name;
      let value = event.target.value;
      let index = event.target.dataset.index;

      // TODO: null vs ""?
      // TODO: 0 vs "0"?
      this.$set(this.configValues[name], index, value);
      
      // required to allow re-submit after validation errors
      event.target.setCustomValidity("");
    },
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadConfig')
      } finally {
        this.loading = false;
      }

      this.updateConfigValues(this.$store.state.config);
    },
    splitlines(description) {
      if (!description) {
        return [];
      }

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
      let field = this.fieldName(mod, tab);

      if (!this.configValues) {
        return null;
      }

      return this.configValues[field];
    },
    fieldErrors(mod, tab) {
      let name = this.fieldName(mod, tab);
      let error = this.configErrors ? this.configErrors[name] : null

      if (error) {
        return [error];
      } else if (tab.validation_errors && tab.validation_errors.length > 0) {
        return tab.validation_errors;
      } else {
        return null;
      }
    },

    toggleModule(mod) {
      let name = this.modName(mod);

      if (this.showModules[name]) {
        this.$set(this.showModules, name, false);
      } else {
        this.$set(this.showModules, name, true);
      }
    },
    showModule(mod) {
      let name = this.modName(mod);

      return this.showModules[name];
    },

    submitErrors(form, errors) {
      let configErrors = {}

      if (errors.set_errors) {
        for (const e of errors.set_errors) {
          if (e.module && e.name) {
            let module = e.module;
            let field = '[' + module + ']' + e.name;
            let input = form.elements[field];

            if (input) {
              input.setCustomValidity(e.error);
            }

            configErrors[field] = e.error;
            this.$set(this.showModules, module, true);
          }
        }
      }

      if (errors.validation_errors) {
        for (const e of errors.validation_errors) {
          let module = e.module + (e.index ? e.index : '');
          let field = '[' + module + ']' + e.name;
          let input = form.elements[field];

          if (input) {
            input.setCustomValidity(e.error);
          }

          configErrors[field] = e.error;
          this.$set(this.showModules, module, true);
        }
      }

      this.configErrors = configErrors;

      // let DOM update to expand modules
      this.$nextTick(() => {
        form.reportValidity();
      });
    },
    async submit(event) {
      const form = event.target;
      const formdata = new FormData(form);

      this.applying = true;
      this.applyError = null;

      try {
        await this.$store.dispatch('postConfig', formdata);
        await this.$store.dispatch('restartSystem');
        await this.$store.dispatch('loadConfig');
      } catch (error) {
        if (error.name == "APIError" && error.data) {
          this.applyError = error.message;
          this.submitErrors(form, error.data);
        } else {
          throw error;
        }
      } finally {
        this.applying = false;
      }
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
