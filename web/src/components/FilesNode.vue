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

  div.vfs-item.vfs-skel {
    border: 1px solid transparent;
  }

  div.vfs-item.vfs-new {
    border: 1px dashed #bbb;
  }

  div.vfs-header {
    display: flex;
    flex-direction: row;
    align-items: center;

    height: 2em;
    padding: 1em;
  }

  div.vfs-item.vfs-skel > div.vfs-header {

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
  <div :class="{'vfs-item': true, 'vfs-root': root, 'vfs-directory': !root }">
    <div class="vfs-header">
      <div class="vfs-controls">
        <button @click="loadSubmit" v-if="!loaded && !loadBusy && !deleteBusy">&searr;</button>

        <progress v-show="loadBusy">Loading...</progress>
        <progress v-show="deleteBusy">Deleting...</progress>
      </div>

      <div class="vfs-title">
        <span v-if="root" class="vfs-name">{{ vfs.path }}</span>
        <span v-else class="vfs-name">{{ name }}/</span>
      </div>

      <div class="vfs-actions">
        <button @click="deleteSubmit" v-if="!root">&times;</button>
      </div>
    </div>

    <div :class="{'vfs-item': true, 'vfs-skel': !mkdir, 'vfs-directory': mkdir, 'vfs-new': mkdir }" v-if="loaded">
      <div class="vfs-header">
        <div class="vfs-controls">
          <button @click="mkdirOpen" v-if="!mkdir">&gt;</button>
          <button @click="mkdirSubmit" v-if="mkdir && !mkdirBusy" :disabled="!mkdirName">&check;</button>
          <progress v-if="mkdir" v-show="mkdirBusy">Creating...</progress>
        </div>

        <div class="vfs-title" v-if="mkdir">
          <input type="text" v-model="mkdirName" />
        </div>

        <div class="vfs-actions">
          <button @click="mkdirCancel" v-if="mkdir">&times;</button>
        </div>
      </div>
    </div>

    <template v-for="(item, i) in items">
      <file-node v-if="item.type == 'file'"
        :vfs="vfs"
        :dir="path"
        :name="item.name"
        :size="item.size"
        :mtime="item.mtime"
      />

      <files-node v-else-if="item.type == 'directory'"
        :vfs="vfs"
        :dir="path"
        :name="item.name"
        :items="item.array"
        :loaded="item.loaded || false"
      />
    </template>

    <div :class="{'vfs-item': true, 'vfs-skel': !upload, 'vfs-file': upload, 'vfs-new': upload }" v-if="loaded">
      <div class="vfs-header">
        <div class="vfs-controls">
          <button @click="uploadOpen" v-if="!upload">&plus;</button>
          <button @click="uploadSubmit" v-if="upload && !uploadBusy" :disabled="!uploadFile">&check;</button>
          <progress v-if="upload" v-show="uploadBusy">Creating...</progress>
        </div>

        <div class="vfs-title" v-if="upload">
          <input type="file" @change="uploadChanged" />
        </div>

        <div class="vfs-actions">
          <button @click="uploadCancel" v-if="upload">&times;</button>
        </div>
      </div>
    </div>
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
      loaded: { type: Boolean, default: false },
    },
    data: () => ({
      loadBusy: false,

      mkdir: false,
      mkdirName: "",
      mkdirBusy: false,

      upload: false,
      uploadFile: null,
      uploadBusy: false,

      deleteBusy: false,
    }),
    computed: {
      root() {
        return !this.dir && !this.name;
      },
      path() {
        if (this.dir) {
          return this.dir + '/' + this.name;
        } else {
          return this.name;
        }
      },
      mkdirPath() {
        const path = this.path;

        if (path) {
          return path + '/' + this.mkdirName;
        } else {
          return this.mkdirName;
        }
      },
      uploadPath() {
        const path = this.path;
        const file = this.uploadFile;

        if (!this.uploadFile) {
          return null;
        } else if (path) {
          return path + '/' + file.name;
        } else {
          return file.name;
        }
      },
    },
    methods: {
      async loadSubmit() {
        const vfsPath = this.vfs.path;
        const path = this.path;

        this.loadBusy = true;

        try {
          await this.$store.dispatch('loadVFSDirectory', { vfsPath, path });
        } catch (error) {
          // TODO:
          throw error;
        } finally {
          this.loadBusy = false;
        }
      },

      /* mkdir */
      mkdirOpen() {
        this.mkdir = true;
      },
      async mkdirSubmit() {
        const vfsPath = this.vfs.path;
        const path = this.mkdirPath;

        this.mkdirBusy = true;

        try {
          await this.$store.dispatch('createVFSDirectory', { vfsPath, path });
        } catch (error) {
          // TODO: input validity
          throw error;
        } finally {
          this.mkdirBusy = false;
        }

        this.mkdir = false;
      },
      mkdirCancel() {
        this.mkdir = false;
      },

      /* file upload */
      uploadOpen() {
        this.upload = true;
      },
      uploadChanged() {
        const input = event.target;
        let file = null

        if (input.files.length > 0) {
          file = input.files[0];
        }

        this.uploadFile = file;
      },
      async uploadSubmit() {
        const vfsPath = this.vfs.path;
        const path = this.uploadPath;
        const file = this.uploadFile;

        this.uploadBusy = true;

        try {
          await this.$store.dispatch('uploadFile', { vfsPath, path, file });
        } catch (error) {
          // TODO: input validity
          throw error;
        } finally {
          this.uploadBusy = false;
        }

        this.upload = false;
      },
      uploadCancel() {
        this.upload = false;
      },

      /* delete */
      async deleteSubmit() {
        const vfsPath = this.vfs.path;
        const path = this.path;

        this.deleteBusy = true;

        try {
          await this.$store.dispatch('deleteDirectory', { vfsPath, path });
        } catch (error) {
          // TODO: input validity
          throw error;
        } finally {
          this.deleteBusy = false;
        }
      },
    }
  }
</script>
