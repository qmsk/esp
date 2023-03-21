export default class VFSService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  makeState(items) {
    const state = {
      array: new Array(),
      map: new Map(),
    };

    for (const vfsItem of items) {
      const vfs = this.makeVFS(vfsItem);

      this.setVFSRoot(state, vfs);
    }

    return state;
  }

  setVFSRoot(node, vfs) {
    const name = vfs.path;

    if (node.map.has(name)) {
      node.array.splice(node.array.indexOf(node.map.get(name)), 1, vfs);
      node.map.set(name, vfs);
    } else {
      node.map.set(name, vfs)
      node.array.push(vfs);
    }

    node.array.sort((a, b) => a.path.localeCompare(b.path));
  }

  makeVFS(item) {
    const vfs = {
      path: item.path,
      map: new Map(),
      array: new Array(),
    };

    for (const fileItem of item.files) {
      this.setVFS(vfs, fileItem);
    }

    return vfs;
  }

  makeDirectory(item) {
    const directory = {
      type: 'directory',
      name: item.name,
      loaded: true,
      map: new Map(),
      array: new Array(),
    };

    if (item.files) {
      for (const fileItem of item.files) {
        this.setVFS(directory, fileItem);
      }
    }

    return directory;
  }

  newDirectory(path, items) {
    const directory = {
      type: 'directory',
      name: path,
      loaded: true,
      map: new Map(),
      array: new Array(),
    };

    for (const item of items) {
      this.setVFS(directory, item);
    }

    return directory;
  }

  setVFS(node, item) {
    if (item.type == 'directory' && item.name.endsWith('/')) {
      item.name = item.name.slice(0, -1);
    }

    const parts = item.name.split('/');
    const name = item.name = parts.pop();

    if (item.type == 'directory') {
      if (!item.map) {
        item.map = new Map();
      }
      if (!item.array) {
        item.array = new Array();
      }
    }

    if (item.mtime) {
      item.mtime = new Date(item.mtime);
    }

    for (const part of parts) {
      if (!node.map.has(part)) {
        const directory = this.newDirectory(part, []);

        node.map.set(part, directory);
        node.array.push(directory);

        node = directory;
      } else {
        node = node.map.get(part);
      }
    }

    if (node.map.has(name)) {
      node.array.splice(node.array.indexOf(node.map.get(name)), 1, item);
      node.map.set(name, item);
    } else {
      node.map.set(name, item);
      node.array.push(item);
    }
  }

  delVFS(vfs, path) {
    const parts = path.split('/');
    const name = parts.pop();

    let node = vfs;

    for (const part of parts) {
      node = node.map.get(part);
    }

    if (node.map.has(name)) {
      node.array.splice(node.array.indexOf(node.map.get(name)), 1);
      node.map.delete(name);
    }
  }

  async get() {
    const response = await this.apiService.get('/vfs');
    const items = response.data;

    return this.makeState(items)
  }

  async getRoot(vfsPath) {
    const response = await this.apiService.get('/vfs' + vfsPath);
    const item = response.data;

    return this.makeVFS(item)
  }

  async getDirectory(vfsPath, path) {
    const response = await this.apiService.get('/vfs' + vfsPath + '/' + path + '/');
    const item = response.data

    return this.makeDirectory(item);
  }

  async createDirectory(vfsPath, path) {
    try {
      const response = await this.apiService.post('/vfs' + vfsPath + '/' + path + '/');
      const item = response.data;

      return this.makeDirectory(item);
    } catch (error) {
      if (error.response && error.response.status == 405) {
        // not supported by filesystem, fake it
        return this.newDirectory(path, []);
      } else {
        throw error;
      }
    }
  }

  async uploadFile(vfsPath, path, file) {
    const response = await this.apiService.putFile('/vfs' + vfsPath + '/' + path, file);
    const item = response.data;

    return item;
  }

  async delete(vfsPath, path) {
    const response = await this.apiService.delete('/vfs' + vfsPath + '/' + path);
  }

  async deleteDirectory(vfsPath, path) {
    const response = await this.apiService.delete('/vfs' + vfsPath + '/' + path + '/');
  }
}
