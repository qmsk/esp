<style>
</style>
<template>
  <button
    @mousedown.left="mousedown"
    @mouseup.left="mouseup"
    @touchstart="mousedown"
    @touchend="mouseup"
    @touchcancel="mouseup"
    :class="{active: this.active}"
  >

  </button>
</template>
<script>
export default {
  props: [
    'pressAction',
    'holdAction',
    'releaseAction',
  ],
  data: () => ({
    active: false,
    holdActive: false,
    holdTimer: null,
    holdTimeout: 500, // ms
  }),
  methods: {
    setHold() {
      if (this.holdTimer) {
        window.clearTimeout(this.holdTimer);
      }

      this.holdTimer = window.setTimeout(() => {
        this.holdTimer = null;

        this.mousehold();
      }, this.holdTimeout);
    },
    clearHold() {
      if (this.holdTimer) {
        window.clearTimeout(this.holdTimer);
      }

      this.holdTimer = null;
    },

    async mousedown() {
      this.setHold();

      if (this.pressAction) {
        this.active = true;

        try {
          await this.$store.dispatch(this.pressAction);
        } finally {
          this.active = false;
        }
      }
    },
    async mousehold() {
      this.holdActive = true;

      this.setHold();

      if (this.holdAction) {
        this.active = true;

        try {
          await this.$store.dispatch(this.holdAction);
        } finally {
          this.active = false;
        }
      }
    },
    async mouseup() {
      if (this.holdTimer) {
        this.clearHold();
      }
      if (!this.holdActive) {
        return;
      }

      this.holdActive = false;

      if (this.releaseAction) {
        this.active = true;

        try {
          await this.$store.dispatch(this.releaseAction);
        } finally {
          this.active = false;
        }
      }
    },
  }
}
</script>
