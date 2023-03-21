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

    padding: 0.5em;

    font-style: italic;
  }

  div.vfs-progress {
    flex: 1 0 0;

    padding: 0.5em;

    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 0.5em;
  }

  div.vfs-progress span {
    flex: 0 0 auto;
  }
  div.vfs-progress meter {
    flex: 1 0 auto;
  }

  div.vfs-attr meter {
    width: 100%;
  }

  div.vfs-actions {
    flex: 0 0 8em;

    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 0.5em;
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
  <div :class="{'vfs-item': true, 'vfs-root': isRoot, 'vfs-directory': !isRoot }">
    <div class="vfs-header">
      <div class="vfs-controls">
        <button @click="loadSubmit" v-if="!isLoaded && !loadBusy && !deleteBusy">
          <span class="material-icons-outlined">expand_more</span>
        </button>
        <button @click="loadCancel" v-if="isLoaded && !loadBusy && !deleteBusy">
          <span class="material-icons-outlined">expand_less</span>
        </button>

        <progress v-show="loadBusy">Loading...</progress>
        <progress v-show="deleteBusy">Deleting...</progress>
      </div>

      <div class="vfs-title">
        <span v-if="isRoot" class="vfs-name">{{ vfs.path }}</span>
        <span v-else class="vfs-name">{{ name }}/</span>
      </div>

      <div class="vfs-attr" v-if="isRoot">
        <meter v-if="vfs.stat"
          min="0"
          :max="statTotalSize"
          :value="statUsedSize"
          :title="statUsedSize | fileSize">

          {{ statUsedSize | fileSize }} used / {{ statTotalSize | fileSize }} total
        </meter>
      </div>

      <div class="vfs-attr" v-if="isRoot">
        <span class="vfs-stat" v-if="vfs.stat">{{ statFreeSize | fileSize }} Free</span>
      </div>

      <div class="vfs-actions">
        <button @click="deleteSubmit" v-if="!isRoot">
          <span class="material-icons-outlined">folder_delete</span>
        </button>
      </div>
    </div>

    <div :class="{'vfs-item': true, 'vfs-skel': !mkdir, 'vfs-directory': mkdir, 'vfs-new': mkdir }" v-if="isLoaded">
      <form>
        <div class="vfs-header">
          <div class="vfs-controls">
            <button type="button" @click="mkdirOpen" v-if="!mkdir">
              <span class="material-icons-outlined">create_new_folder</span>
            </button>
            <button type="submit" @click="mkdirSubmit" v-if="mkdir && !mkdirBusy" :disabled="!mkdirName">
              <span class="material-icons-outlined">create_new_folder</span>
            </button>
            <progress v-if="mkdir" v-show="mkdirBusy">Creating...</progress>
          </div>

          <div class="vfs-title" v-if="mkdir">
            <input type="text" v-model="mkdirName" ref="mkdirInput" />
          </div>

          <div class="vfs-actions">
            <button type="reset" @click="mkdirCancel" v-if="mkdir">
              <span class="material-icons-outlined">cancel</span>
            </button>
          </div>
        </div>
      </form>
    </div>

    <template v-if="isLoaded" v-for="(item, i) in items">
      <file-node v-if="item.type == 'file'" :key="item.name"
        :vfs="vfs"
        :dir="path"
        :name="item.name"
        :size="item.size"
        :mtime="item.mtime"
      />

      <files-node v-else-if="item.type == 'directory'" :key="item.name"
        :vfs="vfs"
        :dir="path"
        :name="item.name"
        :items="item.array"
        :loaded="item.loaded || false"
      />
    </template>

    <div :class="{'vfs-item': true, 'vfs-skel': !upload, 'vfs-file': upload, 'vfs-new': upload }" v-if="isLoaded">
      <form>
        <div class="vfs-header">
          <div class="vfs-controls">
            <button type="button" @click="uploadOpen" v-if="!upload">
              <span class="material-icons-outlined">upload_file</span>
            </button>
            <button type="submit" @click="uploadSubmit" v-if="upload && !uploadBusy" :disabled="!uploadFile">
              <span class="material-icons-outlined">upload_file</span>
            </button>
            <progress v-if="upload" v-show="uploadBusy">Creating...</progress>
          </div>

          <div class="vfs-title" v-if="upload">
            <input type="file" @input="uploadInput" ref="uploadInput" />
          </div>

          <div class="vfs-progress" v-if="uploadFile">
            <span class="vfs-file-size" v-if="uploadProgress">{{ uploadProgress.loaded | fileSize }}</span>
            <meter v-if="uploadFile && uploadProgress"
              min="0"
              :max="uploadProgress.total"
              :value="uploadProgress.loaded"
              :title="uploadProgress.loaded | fileSize">

              {{ uploadProgress.loaded | fileSize }} / {{ uploadProgress.total | fileSize }}
            </meter>
            <span class="vfs-file-size">{{ uploadFileSize | fileSize }}</span>
          </div>
          <div class="vfs-attr" v-if="uploadFile">
            <span class="vfs-file-mtime">{{ uploadFileLastModified | fileTime }}</span>
          </div>

          <div class="vfs-actions">
            <button type="reset" @click="uploadCancel" v-if="upload">
              <span class="material-icons-outlined">cancel</span>
            </button>
          </div>
        </div>
      </form>
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
      load: true,
      loadBusy: false,

      mkdir: false,
      mkdirName: "",
      mkdirBusy: false,

      upload: false,
      uploadFile: null,
      uploadBusy: false,
      uploadProgress: null,

      deleteBusy: false,
    }),
    computed: {
      isRoot() {
        return !this.dir && !this.name;
      },
      isLoaded() {
        return this.loaded && this.load;
      },
      path() {
        if (this.dir) {
          return this.dir + '/' + this.name;
        } else {
          return this.name;
        }
      },
      statTotalSize() {
        const stat = this.vfs.stat;

        return stat.total_sectors * stat.sector_size;
      },
      statUsedSize() {
        const stat = this.vfs.stat;

        return stat.used_sectors * stat.sector_size;
      },
      statFreeSize() {
        const stat = this.vfs.stat;

        return stat.free_sectors * stat.sector_size;
      },
      mkdirPath() {
        const path = this.path;

        if (path) {
          return path + '/' + this.mkdirName;
        } else {
          return this.mkdirName;
        }
      },
      uploadFilePath() {
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
      uploadFileSize() {
        const file = this.uploadFile;

        if (!file) {
          return null;
        }

        return file.size;
      },
      uploadFileLastModified() {
        const file = this.uploadFile;

        if (!file) {
          return null;
        }

        return new Date(file.lastModified);
      },
    },
    methods: {
      async loadSubmit() {
        const vfsPath = this.vfs.path;

        this.load = true;
        this.loadBusy = true;

        try {
          if (this.isRoot) {
            await this.$store.dispatch('loadVFSRoot', { vfsPath});
          } else {
            const path = this.path;

            await this.$store.dispatch('loadVFSDirectory', { vfsPath, path });
          }
        } catch (error) {
          // TODO:
          throw error;
        } finally {
          this.loadBusy = false;
        }
      },
      loadCancel() {
        this.load = false;
      },

      /* mkdir */
      mkdirOpen() {
        this.mkdir = true;
        this.$nextTick(() => this.$refs.mkdirInput.focus());
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
        this.mkdirName = "";
      },
      mkdirCancel() {
        this.mkdir = false;
        this.mkdirName = "";
      },

      /* file upload */
      uploadOpen() {
        this.upload = true;
      },
      async uploadInput(event) {
        const input = event.target;

        if (input.files.length > 0) {
          this.uploadFile = input.files[0];
        } else {
          return;
        }

        this.uploadSubmit();
      },
      async uploadSubmit() {
        const vfsPath = this.vfs.path;
        const file = this.uploadFile;
        const path = this.uploadFilePath;
        let progress = { total: file.size, loaded: 0 };

        this.uploadBusy = true;
        this.uploadProgress = progress

        try {
          await this.$store.dispatch('uploadFile', { vfsPath, path, file, progress });
        } catch (error) {
          // TODO: input validity
          throw error;
        } finally {
          this.uploadBusy = false;
        }

        this.upload = false;
        this.uploadFile = null;
      },
      uploadCancel() {
        this.upload = false;
        this.uploadFile = null;
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
