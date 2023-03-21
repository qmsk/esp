<style>
</style>
<template>
  <div class="vfs-item vfs-file">
    <div class="vfs-header">
      <div class="vfs-controls">
        <progress v-show="uploading">Uploading...</progress>
        <progress v-show="deleting">Deleting...</progress>
      </div>

      <div class="vfs-title">
        <template v-if="temp">
          <input type="file" @input="uploadFile" />
        </template>
        <template v-else>
          {{ name }}
        </template>
      </div>

      <div class="vfs-attr">
        <span class="vfs-file-size" v-if="size">{{ size | fileSize }}</span>
      </div>
      <div class="vfs-attr">
        <span class="vfs-file-mtime" v-if="mtime">{{ mtime | fileTime }}</span>
      </div>

      <div class="vfs-actions">
        <button @click="deleteFile()" v-if="!temp">&times;</button>
        <button @click="cancelTemp()" v-if="temp">&minus;</button>
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
      temp: { type: Boolean, default: false },
    },
    data: function() {
      return {
        uploading: false,
        deleting: false,
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
    methods: {
      buildPath(name) {
        if (this.dir) {
          return this.dir + '/' + name;
        } else {
          return name;
        }
      },
      cancelTemp() {
        if (this.temp) {
          this.$emit('clear');
        }
      },
      async uploadFile(event) {
        const input = event.target;
        const file = input.files[0];
        const path = this.buildPath(file.name)
        const vfsPath = this.vfs.path;

        this.uploading = true;

        try {
          await this.$store.dispatch('uploadFile', { vfsPath, path, file });
        } catch (error) {
          input.setCustomValidity(error.name + ": " + error.message);
          throw error;
        } finally {
          this.uploading = false;
        }

        this.$emit('clear');
      },
      async deleteFile(event) {
        const vfsPath = this.vfs.path;
        const path = this.buildPath(this.name);

        this.deleting = true;

        try {
          await this.$store.dispatch('deleteFile', { vfsPath, path });
        } catch (error) {
          // TODO
          throw error;
        } finally {
          this.deleting = false;
        }
      },
    }
  }
</script>
