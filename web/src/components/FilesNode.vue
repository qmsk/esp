<style>
  div.vfs-item {
    margin-left: 1em;
    margin-top: 1em;
    margin-bottom: 1em;
    padding: 0;

    display: flex;
    flex-direction: column;

    border: 1px solid #bbb;

    color: #111;
  }

  div.vfs-item.vfs-temp {
      border: 1px dashed #bbb;
  }

  div.vfs-header {
    display: flex;
    flex-direction: row;
    align-items: center;

    height: 2em;
    padding: 1em;
  }

  div.vfs-controls {
    flex: 0 0 3em;
  }

  div.vfs-title {
    flex: 1 0 0;

    font-weight: bold;
  }

  div.vfs-attr {
    flex: 1 0 0;

    font-style: italic;
  }

  div.vfs-actions {
    flex: 0 0 8em;
  }

  div.vfs-root {
    background-color: #aaa;
  }

  div.vfs-root > div.vfs-header {
    background-color: #444;
    color: #ccc;
  }

  div.vfs-directory {
    background-color: #888;
    color: #111;
  }

  div.vfs-directory > div.vfs-header {
    background-color: #888;
    color: #111;

    font-weight: bold;
  }

  div.vfs-file {
    background-color: #ccc;
    color: #111;
  }
</style>
<template>
  <div :class="['vfs-item', root ? 'vfs-root' : 'vfs-directory', temp ? 'vfs-temp' : null]">
    <div class="vfs-header">
      <div class="vfs-controls">
        <button @click="load" v-if="!temp && !loaded && !loading && !deleting">&searr;</button>
        <button @click="createDirectory" v-if="temp && !creating">&check;</button>

        <progress v-show="loading">Loading...</progress>
        <progress v-show="creating">Creating...</progress>
        <progress v-show="deleting">Deleting...</progress>
      </div>

      <div class="vfs-title">
        <template v-if="temp">
          <input type="text" v-model="newName" />
        </template>
        <template v-else-if="root">
          <span class="vfs-name">{{ vfs.path }}</span>
        </template>
        <template v-else>
          <span class="vfs-name">{{ name }}/</span>
        </template>
      </div>

      <div class="vfs-actions">
        <button @click="cancel" v-if="temp">&times;</button>
        <button @click="deleteDirectory" v-if="!temp && !root">&times;</button>
        <button @click="newDirectory" v-if="!temp">&gt;</button>
        <button @click="newFile" v-if="!temp">&plus;</button>
      </div>
    </div>


    <template v-for="(item, i) in allItems">
      <file-node v-if="item.type == 'file'"
        :vfs="vfs"
        :dir="path"
        :name="item.name"
        :size="item.size"
        :mtime="item.mtime"
        :temp="item.temp || false"
        @clear="remove(i)"
      />

      <files-node v-else-if="item.type == 'directory'"
        :vfs="vfs"
        :dir="path"
        :name="item.name"
        :items="item.array"
        :temp="item.temp || false"
        :loaded="item.loaded || false"
        @clear="remove(i)"
      />
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
      name: { type: String },
      items: { type: Array, default: () => [] },
      temp: { type: Boolean, default: false },
      loaded: { type: Boolean, default: false },
    },
    data: () => ({
      loading: false,
      creating: false,
      deleting: false,
      newItems: [],
      newName: null,
    }),
    computed: {
      root() {
        return !this.dir && !this.name && !this.temp;
      },
      path() {
        const name = this.temp ? this.newName : this.name;

        if (this.dir) {
          return this.dir + '/' + name;
        } else {
          return name;
        }
      },
      allItems() {
        return [].concat(this.newItems, this.items);
      },
    },
    methods: {
      async load() {
        const vfsPath = this.vfs.path;
        const path = this.path;

        this.loading = true;

        try {
          await this.$store.dispatch('loadVFSDirectory', { vfsPath, path });
        } catch (error) {
          // TODO:
          throw error;
        } finally {
          this.loading = false;
        }
      },
      cancel() {
        this.$emit('clear');
      },
      async createDirectory() {
        const vfsPath = this.vfs.path;
        const path = this.path;

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
        const path = this.path;

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
