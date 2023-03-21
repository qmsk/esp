<style>
</style>
<template>
  <div class="vfs-item vfs-file">
    <div class="vfs-header">
      <div class="vfs-controls">
        <progress v-show="deleteBusy">Deleting...</progress>
      </div>

      <div class="vfs-title">
          <span class="vfs-name">{{ name }}</span>
      </div>

      <div class="vfs-attr">
        <span class="vfs-file-size" v-if="size">{{ size | fileSize }}</span>
      </div>
      <div class="vfs-attr">
        <span class="vfs-file-mtime" v-if="mtime">{{ mtime | fileTime }}</span>
      </div>

      <div class="vfs-actions">
        <button @click="deleteSubmit()">&times;</button>
      </div>
    </div>
  </div>
</template>
<script>
  export default {
    props: {
      vfs: { type: Object },
      dir: { type: String },
      name: { type: String },
      size: { type: Number, default: null },
      mtime: { type: Date, default: null },
    },
    data: function() {
      return {
        deleteBusy: false,
      }
    },
    filters: {
      fileSize(size) {
        const sizeMap = new Map([
          ['GB', 1024 * 1024 * 1024 ],
          ['MB', 1024 * 1024 ],
          ['KB', 1024 ],
        ]);

        for (const [suffix, unit] of sizeMap) {
          if (size >= unit) {
            const units = size / unit;

            return units.toFixed(2) + ' ' + suffix;
          }
        }

        return size.toFixed(0) +  ' ' + 'B';
      },
      fileTime(date) {
        return date.toLocaleString();
      },
    },
    computed: {
      path() {
        if (this.dir) {
          return this.dir + '/' + this.name;
        } else {
          return this.name;
        }
      },
    },
    methods: {
      async deleteSubmit() {
        const vfsPath = this.vfs.path;
        const path = this.path;

        this.deleteBusy = true;

        try {
          await this.$store.dispatch('deleteFile', { vfsPath, path });
        } catch (error) {
          // TODO
          throw error;
        } finally {
          this.deleteBusy = false;
        }
      },
    }
  }
</script>
