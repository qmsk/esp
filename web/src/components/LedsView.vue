<style>
  table.limits td.name {
    text-align: left;
  }
  table.limits td {
    text-align: right;
  }

</style>
<template>
  <main id="leds-view" class="tabbed">
    <nav>
      <a
        v-for="id in ledsKeys"
        :class="{active: id == activeID}"
        @click="switchActive(id)"
      >
        {{ id }}
      </a>
    </nav>
    <div class="view centered">
      <div class="header">
        <h1>LEDS ({{ activeID }})</h1>
        <progress v-show="loading">Loading...</progress>
        <button @click="load"><span :class="{spin: true, active: loading}">&#10227;</span></button>
      </div>

      <template v-if="options">
        <h2>Options</h2>
        <dl>
          <dt>Interface</dt>
          <dd>{{ options.interface }}</dd>

          <dt>Protocol</dt>
          <dd>{{ options.protocol }}</dd>

          <dt>Parameter Type</dt>
          <dd>{{ options.parameter_type }}</dd>

          <dt>Count</dt>
          <dd>{{ options.count }}</dd>

          <dt>Limit Total</dt>
          <dd>{{ options.limit_total }}</dd>

          <dt>Limit Group</dt>
          <dd>{{ options.limit_group }}</dd>

          <dt>Limit Groups</dt>
          <dd>{{ options.limit_groups }}</dd>
        </dl>
      </template>

      <template v-if="status">
        <h2>Status</h2>
        <dl>
          <dt>Active</dt>
          <dd>{{ status.active }}</dd>

          <dt>Last Update</dt>
          <dd>{{ status.update_ms | interval('ms') }}</dd>

          <dt>Test Mode</dt>
          <dd>{{ status.test_mode }}</dd>

        </dl>
      </template>

      <template v-if="status">
        <table class="limits">
          <caption>
            Limit Status
          </caption>
          <thead>
            <tr>
              <th>Group</th>
              <th>Count</th>
              <th>Power</th>
              <th>Limit</th>
              <th>Utilization</th>
              <th>Applied</th>
              <th>Output</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(limit, index) in status.limit_groups">
              <td class="name">Group {{ index }}</td>
              <td>{{ limit.count }}</td>
              <td>{{ limit | limitPower | percentage }}</td>
              <td>{{ limit | limitConfig | percentage }}</td>
              <td>{{ limit | limitUtil | percentage }}</td>
              <td>{{ limit | limitApplied | percentage }}</td>
              <td>{{ limit | limitOutput | percentage }}</td>
              <td>
                <meter min="0" :low="0" optimum="0" :high="limit.limit" :max="limit.count" :value="limit.power" :title="limit | limitUtil | percentage"></meter>
              </td>
            </tr>
            <tr>
              <td class="name">Total</td>
              <td>{{ status.limit_total.count }}</td>
              <td>{{ status.limit_total | limitPower | percentage }}</td>
              <td>{{ status.limit_total | limitConfig | percentage }}</td>
              <td>{{ status.limit_total | limitUtil | percentage }}</td>
              <td>{{ status.limit_total | limitApplied | percentage }}</td>
              <td>{{ status.limit_total | limitOutput | percentage }}</td>
              <td>
                <meter min="0" :low="0" optimum="0" :high="status.limit_total.limit" :max="status.limit_total.count" :value="status.limit_total.power" :title="status.limit_total | limitUtil | percentage"></meter>
              </td>
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
    activeID: "leds1",
  }),
  created() {
    this.load();
  },
  computed: {
    ledsKeys() {
      if (this.$store.state.leds) {
        return [...this.$store.state.leds.keys()];
      }
    },
    options() {
      if (this.$store.state.leds && this.activeID) {
        return this.$store.state.leds.get(this.activeID).options;
      }
    },
    status() {
      if (this.$store.state.leds && this.activeID) {
        return this.$store.state.leds.get(this.activeID).status;
      }
    },
  },
  filters: {
    limitPower: function(limit) {
      if (limit.power && limit.count) {
        return limit.power / limit.count;
      } else {
        return 0.0;
      }
    },
    limitConfig: function(limit) {
      if (limit.limit && limit.count) {
        return limit.limit / limit.count;
      } else {
        return 1.0;
      }
    },
    limitUtil: function(limit) {
      if (limit.power && limit.limit) {
        return limit.power / limit.limit;
      } else {
        return 0.0;
      }
    },
    limitApplied: function(limit) {
      if (limit.output && limit.power) {
        return limit.output / limit.power;
      } else {
        return 1.0;
      }
    },
    limitOutput: function(limit) {
      if (limit.output && limit.limit) {
        return limit.output / limit.limit;
      } else {
        return 0.0;
      }
    },
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadLeds');
      } finally {
        this.loading = false;
      }
    },
    switchActive(id) {
      this.activeID = id;
    },
  }
}
</script>
