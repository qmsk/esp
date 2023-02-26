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
      <button @click="newDirectory">&nearr;</button>
      <button @click="newFile">&plus;</button>
      <button @click="cancelTemp" v-if="temp">&minus;</button>
    </div>

    <template v-for="(item, i) in items">
      <file-node v-if="item.type == 'file'"
        :vfs="vfs"
        :dir="dir"
        :node="item"
        :temp="item.temp || false"
        @remove="remove(i)"
      />

      <div class="vfs-item vfs-directory" v-if="item.type == 'directory'">
        <template v-if="item.temp">
          <input type="text" v-model="item.name" />
        </template>
        <template v-else>
          {{ item.name }}/
        </template>

        <files-node
          :vfs="vfs"
          :dir="buildPath(item.name)"
          :nodes="Array.from(item.tree.values())"
          :temp="item.temp || false"
          @remove="remove(i)"
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
      nodes: { type: Array, default: [] },
      temp: { type: Boolean, default: false },
    },
    data: function () {
      return {
        items: this.nodes,
      };
    },
    computed: {

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
        this.$emit('cancel');
      },
      newDirectory() {
        this.items.push({ type: 'directory', temp: true, tree: new Map() });
      },
      newFile() {
        this.items.push({ type: 'file', temp: true });
      },
      remove(i) {
        this.items.splice(i, 1);
      },
    }
  }
</script>
