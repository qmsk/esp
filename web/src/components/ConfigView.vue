<style>

</style>

<template>
  <main id="config-view" class="view">
    <template v-if="config">
      <h1>Configuration</h1>
      <form action="/api/config" method="post">
        <fieldset v-for="mod in config.modules" :key="mod.name">
          <legend>{{ mod.name }}</legend>

          <template v-for="tab in mod.table">
            <label :for="mod.name + '-' + tab.name">{{ tab.name }}</label>
            <input v-if="tab.type == 'uint16'" type="number"
              :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)"
              :value="tab.value.uint16" min="0" max="65536"
              :readonly="tab.readonly">
            <input v-if="tab.type == 'string' && !tab.secret" type="text"
              :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)"
              :value="tab.value.string"
              :readonly="tab.readonly">
            <input v-if="tab.type == 'string' && tab.secret" type="password"
              :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)"
              :value="tab.value.string"
              :readonly="tab.readonly">
            <input v-if="tab.type == 'bool'" type="hidden"
              :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)"
              :value="false"
              >
            <input v-if="tab.type == 'bool'" type="checkbox"
              :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)"
              :value="true" :checked=tab.value.bool
              :readonly="tab.readonly">

            <select v-if="tab.type == 'enum'"
              :id="mod.name + '-' + tab.name" :name="fieldName(mod, tab)"
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
    <template v-else>
      Loading...
    </template>
  </main>
</div>
</template>

<script>
export default {
  created() {
    this.load()
  },
  computed: {
    config() {
      return this.$store.state.config
    }
  },
  methods: {
    load() {
      this.$store.dispatch('loadConfig')
    },
    fieldName(mod, tab) {
      return '[' + mod.name + ']' + tab.name;
    },
  }

}
</script>
