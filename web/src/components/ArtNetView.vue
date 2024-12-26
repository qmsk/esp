<style>
  table.inputs td,
  table.outputs td {
    text-align: center;
  }
</style>
<template>
  <main id="artnet-view" class="centered">
    <div class="view">
      <div class="header">
        <h1>Art-Net</h1>
        <progress v-show="loading">Loading...</progress>
        <button @click="load"><span :class="{spin: true, active: loading}">&#10227;</span></button>
      </div>

      <template v-if="artnet && artnet.config">
        <h2>Config</h2>
        <dl>
          <dt>Port</dt>
          <dd>{{ artnet.config.port }}</dd>

          <dt>Net</dt>
          <dd>{{ artnet.config.net }}</dd>

          <dt>Subnet</dt>
          <dd>{{ artnet.config.subnet }}</dd>
        </dl>
      </template>

      <template v-if="artnet && artnet.metadata">
        <h2>Metadata</h2>
        <dl>
          <dt>IP</dt>
          <dd>{{ artnet.metadata.ip_address }}</dd>

          <dt>MAC</dt>
          <dd>{{ artnet.metadata.mac_address }}</dd>

          <dt>Short Name</dt>
          <dd>{{ artnet.metadata.short_name }}</dd>

          <dt>Long Name</dt>
          <dd>{{ artnet.metadata.long_name }}</dd>
        </dl>
      </template>

      <template v-if="artnet && artnet.inputs">
        <h2>Inputs</h2>
        <table class="inputs">
          <thead>
            <tr>
              <th>Port</th>
              <th>Index</th>
              <th>Net</th>
              <th>Subnet</th>
              <th>Universe</th>
              <th>Tick</th>
              <th>Length</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="input in artnet.inputs">
              <td>{{ input.port }}</td>
              <td>{{ input.index }}</td>
              <td>{{ input.net }}</td>
              <td>{{ input.subnet }}</td>
              <td>{{ input.universe }}</td>
              <td>{{ input.state.tick }}</td>
              <td>{{ input.state.len }}</td>
            </tr>
          </tbody>
        </table>
      </template>


      <template v-if="artnet && artnet.outputs">
        <h2>Outputs</h2>
        <table class="outputs">
          <thead>
            <tr>
              <th>Port</th>
              <th>Index</th>
              <th>Name</th>
              <th>Net</th>
              <th>Subnet</th>
              <th>Universe</th>
              <th>Tick</th>
              <th>Seq</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="output in artnet.outputs">
              <td>{{ output.port }}</td>
              <td>{{ output.index }}</td>
              <td>{{ output.name }}</td>
              <td>{{ output.net }}</td>
              <td>{{ output.subnet }}</td>
              <td>{{ output.universe }}</td>
              <td>{{ output.state.tick }}</td>
              <td>{{ output.state.seq }}</td>
            </tr>
          </tbody>
        </table>
      </template>
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
    artnet() {
      if (this.$store.state.artnet) {
        return this.$store.state.artnet;
      }
    },
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadArtNet');
      } finally {
        this.loading = false;
      }
    },
  }
}
</script>
