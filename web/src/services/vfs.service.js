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

      state.array.push(vfs);
      state.map.set(vfsItem.path, vfs)
    }

    state.array.sort((a, b) => a.path.localeCompare(b.path));

    return state;
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

  makeDirectory(path, items) {
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

    for (const part of parts) {
      if (!node.map.has(part)) {
        const directory = makeDirectory(part, []);

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

  async getDirectory(vfsPath, path) {
    const response = await this.apiService.get('/vfs' + vfsPath + '/' + path + '/');
    const items = response.data

    return this.makeDirectory(path, items);
  }

  async makeDirectory(vfsPath, path) {
    const response = await this.apiService.post('/vfs' + vfsPath + '/' + path + '/');
    const items = response.data

    return this.makeDirectory(path, []);
  }

  async uploadFile(vfsPath, path, file) {
    const response = await this.apiService.putFile('/vfs' + vfsPath + '/' + path, file);

    return { type: 'file', name: path }; // TODO: size, mtime
  }

  async delete(vfsPath, path) {
    const response = await this.apiService.delete('/vfs' + vfsPath + '/' + path);
  }
}
