function vfsTree(vfs) {
  const root = {
    path: vfs.path,
    tree: new Map(),
  };

  for (const item of vfs.files) {
    const path = item.name.split('/');
    const name = path.pop();
    let dir = root;
    let file = {
      type: "file",
      name: name,
      size: item.size,
      mtime: item.mtime,
    };

    for (const part of path) {
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

  return root;
}

export default class VFSService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/vfs');

    return response.data
  }

  async getTree() {
    const data = await this.get();

    return data.map((vfs) => vfsTree(vfs));
  }
}
