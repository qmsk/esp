<style>
  div.row {
    margin: 1em;
    display: flex;
    flex-direction: row;
    justify-content: space-evenly;
    align-items: flex-start;
  }

  div.user-item {
    display: flex;
    flex-direction: column;
    justify-content: flex-start;
    align-items: center;
  }

  div.user-leds-title {
    padding: 0.25em;
    font-size: x-large;
    font-variant-caps: small-caps;
  }

  div.user-leds {
    width: 50px;
    height: 50px;
    border-radius: 50%;
  }

  div.user-leds-state {
    padding: 0.25em;
    font-family: monospace;
    font-size: large;
    font-weight: bold;
  }

  div.user-leds-timestamp {
    padding: 0.25em;
    font-size: large;
    font-style: italic;
  }

  div.user-leds-interval {
    padding: 0.25em;
    font-size: large;
    font-style: italic;
  }

  div.user-leds-power {
    background: radial-gradient(#00ff00 50%, #000000ff 100%);
  }
  div.user-leds-user {
    background: radial-gradient(#0000ff 50%, #000000ff 100%);
  }
  div.user-leds-activity {
    background: radial-gradient(#ffffff 50%, #000000ff 100%);
  }
  div.user-leds-alert {
    background: radial-gradient(#ff0000 50%, #000000ff 100%);
  }

  div.user-leds-idle {
    opacity: 10%;
  }
  div.user-leds-off {
    opacity: 10%;
  }
  div.user-leds-on {
    opacity: 100%;
  }
  div.user-leds-slow {
    animation: 1900ms infinite user-leds-slow;
  }
  div.user-leds-fast {
    animation: 200ms infinite user-leds-fast;
  }
  div.user-leds-flash {
    opacity: 50%;
    animation: 100ms user-leds-flash;
  }
  div.user-leds-pulse {
    animation: 250ms infinite user-leds-pulse;
  }

  /* 100ms on / 1800ms off */
  @keyframes user-leds-slow {
    0% {
      opacity: 50%;
    }

    5% {
      opacity: 100%;
    }

    10% {
      opacity: 50%;
    }

    100% {
      opacity: 50%;
    }
  }

  /* 100ms on / 100ms  */
  @keyframes user-leds-fast {
    0% {
      opacity: 50%;
    }

    50% {
      opacity: 100%;
    }
    
    100% {
      opacity: 50%;
    }
  }

  /* 10ms on  */
  @keyframes user-leds-flash {
    0% {
      opacity: 100%;
    }

    100% {
      opacity: 50%;
    }
  }

  /* 50ms off / 200ms on  */
  @keyframes user-leds-pulse {
    0% {
      opacity: 100%;
    }

    20% {
      opacity: 50%;
    }

    40% {
      opacity: 100%;
    }

    100% {
      opacity: 100%;
    }
  }

  label.user-button-title {
    padding: 0.25em;
    font-size: x-large;
    font-variant-caps: small-caps;
  }

  button.user-button {
    width: 100px;
    height: 100px;
    border: none;
    border-radius: 50%;
    background: radial-gradient(#444 50%, #222222ff 100%);
  }

  button.user-button:hover {
    background: radial-gradient(#444 50%, #000000ff 100%);
  }

  button.user-button:active, button.user-button.active {
    background: radial-gradient(#888 50%, #000000ff 100%);
  }

  button.user-button:disabled {
    background: radial-gradient(#222 50%, #444444ff 100%);
    opacity: 50%;
  }
</style>
<template>
  <main id="main-view">
    <div class="view centered">
      <div class="header">
        <h1>Status</h1>
        <progress v-show="loading">Loading...</progress>
        <button @click="load"><span :class="{spin: true, active: loading}">&#10227;</span></button>
      </div>

      <div class="row" v-if="status">
        <div id="user-power" class="user-item">
          <div class="user-leds-title">Power</div>
          <div class="user-leds user-leds-power" :class="status.power.leds_state | userLedsClass"></div>
          <div class="user-leds-state">{{ status.power.type }}</div>
          <div class="user-leds-timestamp" v-if="status.power.tick">{{ tickToDate(status.power.tick) | timestamp }}</div>
          <div class="user-leds-interval" v-if="status.power.tick_ms">{{ status.power.tick_ms | interval('ms', 's') }}</div>
        </div>

        <div id="user-state" class="user-item">
          <div class="user-leds-title">State</div>
          <div class="user-leds user-leds-user" :class="status.state.leds_state | userLedsClass"></div>
          <div class="user-leds-state">{{ status.state.type }}</div>
          <div class="user-leds-timestamp" v-if="status.state.tick">{{ tickToDate(status.state.tick) | timestamp }}</div>
          <div class="user-leds-interval" v-if="status.state.tick_ms">{{ status.state.tick_ms | interval('ms', 's') }}</div>
        </div>

        <div id="user-activity" class="user-item">
          <div class="user-leds-title">Activity</div>
          <div class="user-leds user-leds-activity" :class="status.activity.leds_state | userLedsClass"></div>
          <div class="user-leds-state">{{ status.activity.type }}</div>
          <div class="user-leds-timestamp" v-if="status.activity.tick">{{ tickToDate(status.activity.tick) | timestamp }}</div>
          <div class="user-leds-interval" v-if="status.activity.tick_ms">{{ status.activity.tick_ms | interval('ms', 's') }}</div>
        </div>

        <div id="user-alert" class="user-item">
          <div class="user-leds-title">Alert</div>
          <div class="user-leds user-leds-alert" :class="status.alert.leds_state | userLedsClass"></div>
          <div class="user-leds-state">{{ status.alert.type }}</div>
          <div class="user-leds-timestamp" v-if="status.alert.tick">{{ tickToDate(status.alert.tick) | timestamp }}</div>
          <div class="user-leds-interval" v-if="status.alert.tick_ms">{{ status.alert.tick_ms | interval('ms', 's') }}</div>
        </div>
      </div>

      <div class="row" v-if="status">
        <div id="user-config" class="user-item">
          <label for="user-config-button" class="user-button-title">Config</label>
          <button id="user-config-button" class="user-button user-button-config"
            :class="{active: configActive}"
            @click="clickConfig"
          >

          </button>
        </div>

        <div id="user-test" class="user-item">
          <label for="user-test-button" class="user-button-title">Test</label>
          <button id="user-test-button" class="user-button user-button-test"
            :class="{active: testActive}"
            @click="clickTest"
          >

          </button>
        </div>
      </div>
    </div>
  </main>
</template>
<script>
export default {
  data: () => ({
    loading: true,
    configActive: false,
    testActive: false,
  }),
  created() {
    this.load();
  },
  computed: {
    status() {
      if (this.$store.state.status) {
        return this.$store.state.status;
      }
    },
  },
  filters: {
    userLedsClass(state) {
      switch(state) {
        case "IDLE":    return "user-leds-idle";
        case "OFF":     return "user-leds-off";
        case "ON":      return "user-leds-on";
        case "SLOW":    return "user-leds-slow";
        case "FAST":    return "user-leds-fast";
        case "FLASH":   return "user-leds-flash";
        case "PULSE":   return "user-leds-pulse";
      }
    }
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadStatus');
      } finally {
        this.loading = false;
      }
    },
    tickToDate(tick) {
      let status = this.$store.state.status;
      let status_timestamp = this.$store.state.status_timestamp;

      if (!status || !status_timestamp) {
        return null;
      }
      if (!tick) {
        return null;
      }

      // UTC ms
      let timestamp = status_timestamp - (status.tick - tick) * status.tick_rate_ms;

      return new Date(timestamp);
    },
    async clickConfig() {
      this.loading = true;
      this.configActive = true;

      try {
        await this.$store.dispatch('pressConfigButton');
      } finally {
        this.loading = false;
        this.configActive = false;
      }
    },
    async clickTest() {
      this.loading = true;
      this.testActive = true;

      try {
        await this.$store.dispatch('pressTestButton');
      } finally {
        this.loading = false;
        this.testActive = false;
      }
    },
  }
}
</script>
