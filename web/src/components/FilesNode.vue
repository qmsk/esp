<style>
  div.vfs-tree {
    border: 1px solid #bbb;
  }

  div.vfs-controls {
    padding: 1em;
  }

  div.vfs-file-controls {
    float: right;
  }

  div.vfs-item {
    margin-left: 1em;
    margin-top: 1em;
    margin-bottom: 1em;
    padding: 1em;

    background-color: #ccc;
    color: #111;
  }

  div.vfs-file {
    background-color: #ccc;
    color: #111;
  }

  div.vfs-directory {
    background-color: #888;
    color: #111;
  }
</style>
<template>
  <div class="vfs-nodes">
    <div class="vfs-controls">
      <button @click="loadDirectory" v-if="!temp && !loaded">&searr;</button>
      <button @click="newDirectory" v-if="!temp">&gt;</button>
      <button @click="newFile" v-if="!temp">&plus;</button>
      <button @click="cancelTemp" v-if="temp">&times;</button>
      <button @click="createDirectory" v-if="temp">&check;</button>
      <button @click="deleteDirectory()" v-if="!temp">&times;</button>
    </div>

    <template v-for="(item, i) in allItems">
      <file-node v-if="item.type == 'file'"
        :vfs="vfs"
        :dir="dir"
        :node="item"
        :temp="item.temp || false"
        @clear="remove(i)"
      />

      <div class="vfs-item vfs-directory" v-if="item.type == 'directory'">
        <template v-if="item.temp">
          <input type="text" v-model="item.name" />
          <progress v-show="creating">Creating...</progress>
        </template>
        <template v-else>
          {{ item.name }}/
          <progress v-show="deleting">Deleting...</progress>
        </template>

        <files-node
          :vfs="vfs"
          :dir="buildPath(item.name)"
          :items="item.array"
          :temp="item.temp || false"
          :loaded="item.loaded || false"
          @clear="remove(i)"
        />
      </div>
    </template>
  </div>
</template>
<script>
  import FileNode from "./FileNode"

  export default {
    name: 'files-node',
    components: {
      FileNode,
    },
    props: {
      vfs: { type: Object },
      dir: { type: String },
      items: { type: Array, default: () => [] },
      temp: { type: Boolean, default: false },
      loaded: { type: Boolean, default: false },
    },
    data: () => ({
      creating: false,
      deleting: false,
      newItems: [],
    }),
    computed: {
      allItems() {
        return [].concat(this.newItems, this.items);
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
      async loadDirectory() {
        const vfsPath = this.vfs.path;
        const path = this.dir;

        await this.$store.dispatch('loadVFSDirectory', { vfsPath, path });
      },
      cancelTemp() {
        this.$emit('clear');
      },
      async createDirectory() {
        const vfsPath = this.vfs.path;
        const path = this.dir;

        this.creating = true;

        try {
          await this.$store.dispatch('createVFSDirectory', { vfsPath, path });
        } catch (error) {
          // TODO: input validity
          throw error;
        } finally {
          this.creating = false;
        }

        this.$emit('clear');
      },
      async deleteDirectory() {
        const vfsPath = this.vfs.path;
        const path = this.dir;

        this.deleting = true;

        try {
          await this.$store.dispatch('deleteDirectory', { vfsPath, path });
        } catch (error) {
          // TODO: input validity
          throw error;
        } finally {
          this.deleting = false;
        }
      },
      newDirectory() {
        this.newItems.push({ type: 'directory', temp: true });
      },
      newFile() {
        this.newItems.push({ type: 'file', temp: true });
      },
      remove(i) {
        this.newItems.splice(i, 1);
      },
    }
  }
</script>
