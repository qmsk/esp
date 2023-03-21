<style>

</style>
<template>
  <main id="files-view" class="split">
    <div class="view">
      <h1>Files</h1>
      <progress v-show="loading">Loading...</progress>

      <files-node v-if="vfsArray" v-for="vfs in vfsArray"
        :vfs="vfs"
        dir=""
        name=""
        :items="vfs.array"
        :loaded="vfs.mounted"
      />
    </div>
  </main>
</template>
<script>
import FilesNode from "./FilesNode"

export default {
  components: {
    FilesNode,
  },
  data: () => ({
    loading: true,
  }),
  created() {
    this.load();
  },
  computed: {
    vfsArray() {
      if (this.$store.state.vfs) {
        return this.$store.state.vfs.array;
      }
    },
  },
  methods: {
    async load() {
      this.loading = true;

      try {
        await this.$store.dispatch('loadVFS');
      } finally {
        this.loading = false;
      }
    },
  }
}
</script>
