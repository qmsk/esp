<style>
table.tasks td {
  text-align: center;
}
table.tasks td.name {
  text-align: left;
}
table.tasks td.cpu,
table.tasks td.stack-free {
  text-align: right;
}
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

          <dt>Reset reason</dt>
          <dd>{{ status.reset_reason }}</dd>
        </dl>

        <h2>Software</h2>
        <dl>
          <dt>SDK</dt>
          <dd>{{ info.sdk_name }} @ version {{ info.sdk_version }}</dd>

          <dt>Application</dt>
          <dd>{{ info.app_name }} @ version {{ info.app_version }}</dd>

          <dt>Build</dt>
          <dd>{{ info.build_date }} @ {{ info.build_time }}</dd>

          <dt>Uptime</dt>
          <dd>{{ status.uptime | uptime }}</dd>
        </dl>

        <h2>Memory</h2>
        <dl>
          <dt>Size</dt>
          <dd>{{ status.heap_size | kib }}</dd>

          <dt>Free</dt>
          <dd>{{ status.heap_free | kib }}</dd>

          <dt>Boot</dt>
          <dd>
            <meter min="0" :max="status.heap_size" :value="status.heap_size - status.heap_free_max" :title="status.heap_size - status.heap_free_max | kib">
              {{ status.heap_size - status.heap_free_max | kib }} used / {{ status.heap_size | kib }} total
            </meter>
          </dd>

          <dt>Current</dt>
          <dd>
            <meter min="0" :max="status.heap_size" :value="status.heap_size - status.heap_free" :title="status.heap_size - status.heap_free | kib">
              {{ status.heap_size - status.heap_free | kib }} used / {{ status.heap_size | kib }} total
            </meter>
          </dd>

          <dt>Maximum</dt>
          <dd>
            <meter min="0" :max="status.heap_size" :value="status.heap_size - status.heap_free_min" :title="status.heap_size - status.heap_free_min | kib">
              {{ status.heap_size - status.heap_free_min | kib }} used / {{ status.heap_size | kib }} total
            </meter>
          </dd>

        </dl>
      </template>
      <template v-for="(info, name) in interfaces">
        <h2>Interface {{ name }} <span class="state">({{ info.state }})</span></h2>
        <dl>
          <template v-if="info.state == 'UP'">
            <dt>Hostname</dt>
            <dd>{{ info.hostname }}</dd>

            <dt>IPv4 Address</dt>
            <dd>{{ info.ipv4_address }}</dd>

            <dt>IPv4 Network</dt>
            <dd>{{ info.ipv4_network }}/{{ info.ipv4_prefixlen }}</dd>

            <dt>IPv4 Gateway</dt>
            <dd>{{ info.ipv4_gateway }}</dd>

            <dt>DNS (IPV4)</dt>
            <dd>{{ info.dns_main }} / {{ info.dns_backup }} / {{ info.dns_fallback }}</dd>
          </template>
        </dl>
      </template>
      <template v-if="partitions">
        <h2>Partitions</h2>
        <table>
          <thead>
            <tr>
              <th>Label</th>
              <th>Type</th>
              <th>SubType</th>
              <th>Start</th>
              <th>End</th>
              <th>Size</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="part in partitions" :key="part.start">
              <td>{{ part.label }}</td>
              <td>{{ part.type }}</td>
              <td>{{ part.subtype }}</td>
              <td class="ptr">{{ part.start | ptr }}</td>
              <td class="ptr">{{ part.end | ptr }}</td>
              <td class="size">{{ part.size | kib }}</td>
            </tr>
          </tbody>
        </table>
      </template>
      <template v-if="tasks">
        <table class="tasks">
          <caption>
            Tasks

            <button @click="loadTasks"><span :class="{spin: true, active: loadingTasks}">&#10227;</span></button>
          </caption>
          <thead>
            <tr>
              <th>#</th>
              <th>Name</th>
              <th>State</th>
              <th>Priority</th>
              <th>CPU now</th>
              <th>CPU total</th>
              <th>Stack</th>
              <th>Stack Usage</th>
              <th>Stack Free</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="task in tasks" :key="task.number">
              <td>{{ task.number }}</td>
              <td class="name">{{ task.name }}</td>
              <td>{{ task.state }}</td>
              <td>
                <span v-if="task.current_priority != task.base_priority">{{ task.base_priority }} &rarr; {{ task.current_priority }}</span>
                <span v-else>{{ task.base_priority }}</span>
              </td>
              <td class="cpu">
                <template v-if="task.prev_runtime">{{ (task.runtime - task.prev_runtime) / (task.total_runtime - task.prev_total_runtime) | percentage }}</template>
              </td>
              <td class="cpu">
                {{ task.runtime / task.total_runtime | percentage }}
              </td>
              <td class="stack">{{ task.stack_size | kib }}</td>
              <td>
                <meter min="0" :max="task.stack_size" :value="task.stack_size - task.stack_highwater_mark">
                  {{ task.stack_size - task.stack_highwater_mark | kib }} used / {{ task.stack_size | kib }} total
                </meter>
              </td>
              <td class="stack">{{ task.stack_highwater_mark | kib }}</td>
            </tr>
          </tbody>
        </table>
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
    loadingTasks: false,
    restarting: false,
  }),
  created() {
    this.load();
  },
  computed: {
    info() {
      if (this.$store.state.system) {
        return this.$store.state.system.info;
      }
    },
    status() {
      if (this.$store.state.system) {
        return this.$store.state.system.status;
      }
    },
    partitions() {
      if (this.$store.state.system) {
        return this.$store.state.system.partitions;
      }
    },
    tasks() {
      if (this.$store.state.system) {
        return this.sortTasks(this.$store.state.system.tasks);
      }
    },
    interfaces() {
      if (this.$store.state.system.interfaces) {
        return this.$store.state.system.interfaces;
      }
    }
  },
  filters: {
    mhz: function(value) {
      return (value / 1000 / 1000).toFixed(1) + ' MHz';
    },
    mib: function(value) {
      return (value / 1024 / 1024).toFixed(1) + ' MiB';
    },
    kib: function(value) {
      return (value / 1024).toFixed(1) + ' KiB';
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
    ptr: function(addr) {
      return "0x" + addr.toString(16).padStart(8, '0');
    },
    percentage: function(value) {
      return (value * 100).toFixed(1) + '%';
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
    async loadTasks() {
      this.loadingTasks = true;

      try {
        await this.$store.dispatch('loadSystemTasks');
      } finally {
        this.loadingTasks = false;
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
    },
    sortTasks(tasks) {
      tasks = Array.from(tasks);
      tasks.sort((a, b) => a.number - b.number);

      return tasks;
    },
  }
}
</script>
