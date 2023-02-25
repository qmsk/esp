<style>
  div.vfs-tree {
    border: 1px solid #bbb;
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
    <template v-for="item in files">
      <div class="vfs-item vfs-file" v-if="item.type == 'file'">
        {{ item.name }}
      </div>

      <div class="vfs-item vfs-directory" v-if="item.type == 'directory'">
        {{ item.name }}/

        <files-node :files="item.tree.values()" />
      </div>
    </template>
  </div>
</template>
<script>

  export default {
    name: 'files-node',
    props: {
      files: { },
    },
  }
</script>
