export default class VFSService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  makeTree(vfs) {
    const root = {
      path: vfs.path,
      tree: new Map(),
    };

    for (const item of vfs.files) {
      this.setFile(root, item);
    }

    return root;
  }

  setFile(vfs, item) {
    const parts = item.name.split('/');
    const name = parts.pop();

    let dir = vfs;
    let file = {
      type: "file",
      name: name,
      size: item.size,
      mtime: item.mtime,
    };

    for (const part of parts) {
      if (!dir.tree.has(part)) {
        dir.tree.set(part, {
          type: "directory",
          name: part,
          tree: new Map(),
        });
      }

      dir = dir.tree.get(part);
    }

    dir.tree.set(name, file);
  }

  removeFile(vfs, path) {
    const parts = path.split('/');
    const name = parts.pop();

    let dir = vfs;

    for (const part of parts) {
      dir = dir.tree.get(part);
    }

    dir.tree.delete(name);
  }

  async get() {
    const response = await this.apiService.get('/vfs');

    return response.data
  }

  async getTree() {
    const data = await this.get();

    return data.map((vfs) => this.makeTree(vfs));
  }

  async upload(vfs, path, file) {
    const response = await this.apiService.putFile('/vfs' + vfs.path + '/' + path, file);

    return { type: 'file', name: path }; // TODO: size, mtime
  }

  async delete(vfs, path) {
    const response = await this.apiService.delete('/vfs' + vfs.path + '/' + path);
  }
}
