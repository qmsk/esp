<style>
  div.vfs-tree {

  }
</style>

<template>
  <main id="files-view" class="split">
    <div class="view">
      <h1>Files</h1>
      <progress v-show="loading">Loading...</progress>
      <template v-if="vfs">
        <div class="vfs-tree" v-for="vfs in vfs">
          <h2>{{ vfs.path }}</h2>
          <files-node :vfs="vfs" dir="" :nodes="Array.from(vfs.tree.values())" />
        </div>
      </template>
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
    vfs() {
      if (this.$store.state.vfs) {
        return this.$store.state.vfs;
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
