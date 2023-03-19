<style>
</style>
<template>
  <div class="vfs-item vfs-file">
    <div class="vfs-file-controls">
      <button @click="deleteFile()" v-if="!temp">&times;</button>
      <button @click="cancelTemp()" v-if="temp">&minus;</button>
    </div>

    <template v-if="temp">
      <input type="file" @input="uploadFile" />
      <progress v-show="uploading">Uploading...</progress>
    </template>
    <template v-else>
      {{ node.name }}
      <progress v-show="deleting">Deleting...</progress>
    </template>
  </div>
</template>
<script>
  export default {
    props: {
      vfs: { type: Object },
      dir: { type: String },
      node: { type: Object },
      temp: { type: Boolean, default: false },
    },
    data: function() {
      return {
        uploading: false,
        deleting: false,
      }
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
        const path = this.buildPath(this.node.name);

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
