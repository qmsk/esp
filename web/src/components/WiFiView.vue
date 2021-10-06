<style>

</style>
<template>
  <main id="config-view" class="split">
    <div class="view">
      <h1>WiFi</h1>
      <progress v-show="loading">Loading...</progress>

      <template v-if="wifi && !wifi.ap">
        <h2>AP <span class="state">(Disabled)</span></h2>
      </template>
      <template v-if="wifi && wifi.ap">
        <h2>AP <span class="state">(Configured)</span></h2>
        <dl>
          <dt>SSID</dt>
          <dd>{{ wifi.ap.config.ssid }}</dd>

          <dt>Channel</dt>
          <dd>{{ wifi.ap.config.channel }}</dd>

          <dt>Authentication</dt>
          <dd>{{ wifi.ap.config.authmode }}</dd>

          <dt>Hidden</dt>
          <dd>{{ wifi.ap.config.ssid_hidden }}</dd>

          <dt>Connections</dt>
          <dd>
            <meter min="0" :max="wifi.ap.config.max_connection" :value="wifi.ap.sta.length" :title="wifi.ap.sta.length">
              {{ wifi.ap.sta.length }} connections / {{ wifi.ap.config.max_connection }} max
            </meter>
          </dd>

          <dt>Beacon Interval</dt>
          <dd>{{ wifi.ap.config.beacon_interval_ms }} ms</dd>
        </dl>
      </template>
      <template v-if="wifi && wifi.ap && wifi.ap.sta">
        <h2>AP STA</h2>
        <table>
          <thead>
            <tr>
              <th>MAC</th>
              <th>IP</th>
              <th>Mode</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="sta in wifi.ap.sta" :key="sta.bssid">
              <td>{{ sta.mac }}</td>
              <td>{{ sta.ipv4 }}</td>
              <td>
                <span v-if="sta.phy_11b">b</span>
                <span v-if="sta.phy_11g">g</span>
                <span v-if="sta.phy_11n">n</span>
                <span v-if="sta.phy_lr">LR</span>
                <span v-if="sta.wps">WPS</span>
              </td>
            </tr>
          </tbody>
        </table>
      </template>

      <template v-if="wifi && !wifi.sta">
        <h2>STA <span class="state">(Disabled)</span></h2>
      </template>
      <template v-if="wifi && wifi.sta && !wifi.sta.ap">
        <h2>STA <span class="state">(Configured)</span></h2>
        <dl>
          <dt>SSID</dt>
          <dd>{{ wifi.sta.config.ssid }}</dd>

          <dt>BSSID</dt>

          <dt>Channel</dt>
          <dd v-if="wifi.sta.config.channel > 0">{{ wifi.sta.config.channel }}</dd>

          <dt>RSSI (threshold)</dt>
          <dd>{{ wifi.sta.config.threshold_rssi }}</dd>

          <dt>Authentication (threshold)</dt>
          <dd>{{ wifi.sta.config.threshold_authmode }}</dd>

          <dt>Cipher</dt>

          <dt>Mode</dt>
        </dl>
      </template>
      <template v-if="wifi && wifi.sta && wifi.sta.ap">
        <h2>STA <span class="state">(Connected)</span></h2>
        <dl>
          <dt>SSID</dt>
          <dd>{{ wifi.sta.ap.ssid }}</dd>

          <dt>BSSID</dt>
          <dd>{{ wifi.sta.ap.bssid }}</dd>

          <dt>Channel</dt>
          <dd>{{ wifi.sta.ap.channel }}</dd>

          <dt>RSSI</dt>
          <dd>{{ wifi.sta.ap.rssi }}</dd>

          <dt>Authentication</dt>
          <dd>{{ wifi.sta.ap.authmode }}</dd>

          <dt>Cipher</dt>
          <dd>{{ wifi.sta.ap.pairwise_cipher }} / {{ wifi.sta.ap.group_cipher }}</dd>

          <dt>Mode</dt>
          <dd>
            <span v-if="wifi.sta.ap.phy_11b">b</span>
            <span v-if="wifi.sta.ap.phy_11g">g</span>
            <span v-if="wifi.sta.ap.phy_11n">n</span>
            <span v-if="wifi.sta.ap.phy_lr">LR</span>
            <span v-if="wifi.sta.ap.wps">WPS</span>
          </dd>
        </dl>
      </template>
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
    wifi() {
      if (this.$store.state.wifi) {
        return this.$store.state.wifi;
      }
    },
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadWiFi');
      } finally {
        this.loading = false;
      }
    },
  }
}
</script>
