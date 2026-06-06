<style>
  table.inputs td,
  table.outputs td {
    text-align: center;
  }
</style>
<template>
  <main id="artnet-view">
    <div class="view centered">
      <h1>
        Art-Net
        <progress v-show="loading">Loading...</progress>
        <button @click="load"><span :class="{spin: true, active: loading}">&#10227;</span></button>
      </h1>

      <template v-if="artnet && artnet.info">
        <h2>Info</h2>
        <dl>
          <dt>Port</dt>
          <dd>{{ artnet.info.port }}</dd>
          
          <dt>IP</dt>
          <dd>{{ artnet.info.ip_address }}</dd>

          <dt>MAC</dt>
          <dd>{{ artnet.info.mac_address }}</dd>

          <dt>Short Name</dt>
          <dd>{{ artnet.info.short_name }}</dd>

          <dt>Long Name</dt>
          <dd>{{ artnet.info.long_name }}</dd>
        </dl>
      </template>

      <template v-if="artnet && artnet.status">
        <h2>Status</h2>
        <dl>
          <dt>Sync Mode</dt>
          <dd>{{ artnet.status.sync_mode }}</dd>
        </dl>
      </template>

      <template v-if="artnet && artnet.metrics">
        <h2>Metrics</h2>
        <dl>
          <dt>Recv</dt>
          <dd><TimerMetric :timerMetric="artnet.metrics.recv_timer" /></dd>

          <dt>Recv (Poll)</dt>
          <dd><CounterMetric :counterMetric="artnet.metrics.recv_poll_counter" /></dd>

          <dt>Recv (DMX)</dt>
          <dd><CounterMetric :counterMetric="artnet.metrics.recv_dmx_counter" /></dd>

          <dt>Recv (Sync)</dt>
          <dd><CounterMetric :counterMetric="artnet.metrics.recv_sync_counter" /></dd>

          <dt>DMX (Discard)</dt>
          <dd><CounterMetric :counterMetric="artnet.metrics.dmx_discard_counter" /></dd>
        </dl>
      </template>

      <template v-if="inputs">
        <table class="inputs">
          <caption>
            Inputs
            <button @click="loadInputs"><span :class="{spin: true, active: loadingInputs}">&#10227;</span></button>
          </caption>
          <thead>
            <tr>
              <th>Name</th>
              <th>Net</th>
              <th>Subnet</th>
              <th>Universe</th>
              <th>Tick</th>
              <th>Length</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="input in inputs">
              <td>{{ input.name }}</td>
              <td>{{ input.net }}</td>
              <td>{{ input.subnet }}</td>
              <td>{{ input.universe }}</td>
              <td>{{ input.state.tick_ms | interval('ms') }}</td>
              <td>{{ input.state.len }}</td>
            </tr>
          </tbody>
        </table>
      </template>

      <template v-if="outputs">
        <table class="outputs">
        <caption>
          Outputs
          <button @click="loadOutputs"><span :class="{spin: true, active: loadingOutputs}">&#10227;</span></button>
        </caption>
          <thead>
            <tr>
              <th>Name</th>
              <th>Net</th>
              <th>Subnet</th>
              <th>Universe</th>
              <th>DMX</th>
              <th>Seq Miss</th>
              <th>Seq Drop</th>
              <th>Update</th>
              <th>Overflow</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="output in outputs">
              <td>{{ output.name }}</td>
              <td>{{ output.net }}</td>
              <td>{{ output.subnet }}</td>
              <td>{{ output.universe }}</td>
              <td><CounterMetric :counterMetric="output.metrics.dmx_counter" /></td>
              <td><CounterMetric :counterMetric="output.metrics.seq_miss_counter" /></td>
              <td><CounterMetric :counterMetric="output.metrics.seq_drop_counter" /></td>
              <td><CounterMetric :counterMetric="output.metrics.update_counter" /></td>
              <td><CounterMetric :counterMetric="output.metrics.overflow_counter" /></td>
            </tr>
          </tbody>
        </table>
      </template>
    </div>
  </main>
</template>
<script>
import CounterMetric from "./CounterMetric"
import TimerMetric from "./TimerMetric"

export default {
  components: {
    CounterMetric,
    TimerMetric,
  },
  data: () => ({
    loading: true,
    loadingInputs: false,
    loadingOutputs: false,
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
    inputs() {
      if (this.$store.state.artnet_inputs) {
        return this.$store.state.artnet_inputs;
      }
    },
    outputs() {
      if (this.$store.state.artnet_outputs) {
        return this.$store.state.artnet_outputs;
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
    async loadInputs() {
      this.loadingInputs = true;

      try {
        await this.$store.dispatch('loadArtNetInputs');
      } finally {
        this.loadingInputs = false;
      }
    },
    async loadOutputs() {
      this.loadingOutputs = true;

      try {
        await this.$store.dispatch('loadArtNetOutputs');
      } finally {
        this.loadingOutputs = false;
      }
    },
  }
}
</script>
