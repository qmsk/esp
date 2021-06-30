<style>

</style>

<template>
  <main id="config-view" class="split">
    <div class="view">
      <h1>System</h1>
      <progress v-show="loading">Loading...</progress>

      <template v-if="info && status">
        <h2>Hardware</h2>
        <dl>
          <dt>Chip</dt>
          <dd>{{ info.chip_model }} @ revision {{ info.chip_revision }}</dd>

          <dt>CPU</dt>
          <dd>{{ info.cpu_cores }} core(s) @ {{ status.cpu_frequency | mhz }}</dd>

          <dt>Flash</dt>
          <dd>{{ info.flash_size | mib }} @ {{ info.flash_usage | mib }} code</dd>

          <dt>iRAM</dt>
          <dd>{{ info.iram_size | kib }} @ {{ info.iram_usage | kib }} code + {{ info.iram_heap | kib }} heap</dd>

          <dt>dRAM</dt>
          <dd>{{ info.dram_size | kib }} @ {{ info.dram_usage | kib }} code + {{ info.dram_heap | kib }} heap</dd>
        </dl>

        <h2>Software</h2>
        <dl>
          <dt>SDK</dt>
          <dd>{{ info.sdk_name }} @ version {{ info.sdk_version }}</dd>

          <dt>Application</dt>
          <dd>{{ info.app_name }} @ version {{ info.app_version }}</dd>

          <dt>Build</dt>
          <dd>{{ info.build_date }} @ {{ info.build_time }}</dd>
        </dl>

        <h2>Operating System</h2>
        <dl>
          <dt>Uptime</dt>
          <dd>{{ status.uptime | uptime }}</dd>

          <dt>Reset reason</dt>
          <dd>{{ status.reset_reason }}</dd>
        </dl>

        <h2>Memory</h2>
        <dl>
          <dt>Size</dt>
          <dd>{{ status.heap_size | kib }}</dd>

          <dt>Free</dt>
          <dd>{{ status.heap_free | kib }}</dd>

          <dt>Usage</dt>
          <dd>
            <meter min="0" :max="status.heap_size" :value="status.heap_size - status.heap_free">
              {{ status.heap_size - status.heap_free | kib }} used / {{ status.heap_size | kib }} total
            </meter>
          </dd>

          <dt>High</dt>
          <dd>
            <meter min="0" :max="status.heap_size" :value="status.heap_size - status.heap_free_min">
              {{ status.heap_size - status.heap_free_min | kib }} used / {{ status.heap_size | kib }} total
            </meter>
          </dd>

        </dl>
      </template>
    </div>
    <div class="controls">
      <form @submit="restartSubmit">
        <fieldset>
          <button type="submit">Restart</button>
          <progress v-show="restarting">Restarting...</progress>
        </fieldset>
      </form>
    </div>
  </main>
</template>

<script>
export default {
  data: () => ({
    loading: true,
    restarting: false,
  }),
  created() {
    this.load();
  },
  computed: {
    info() {
      return this.$store.state.system_info;
    },
    status() {
      return this.$store.state.system_status;
    },
  },
  filters: {
    mhz: function(value) {
      return (value / 1000 / 1000).toFixed(1) + 'MHz';
    },
    mib: function(value) {
      return (value / 1024 / 1024).toFixed(1) + 'MiB';
    },
    kib: function(value) {
      return (value / 1024).toFixed(1) + 'KiB';
    },
    uptime: function(us) {
      const s = Math.floor(us / 1000 / 1000);
      const m = Math.floor(s / 60);
      const h = Math.floor(m / 60);
      const d = Math.floor(h / 24);

      let parts = new Map();

      parts.set('days', d);
      parts.set('hours', h - d * 24);
      parts.set('minutes', m - h * 60);
      parts.set('seconds', s - m * 60);

      let out = [];

      for (let [title, count] of parts) {
        if (count > 0) {
          out.push(count + " " + title);
        }
      }

      return out.join(", ");
    },
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadSystem');
      } finally {
        this.loading = false;
      }
    },
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
