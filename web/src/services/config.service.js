export default class ConfigService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/api/config');

    return response.data
  }

  async post(config, formdata) {
    let data = new URLSearchParams();

    for (let mod of config.modules) {
      for (let tab of mod.table) {
        let modname = mod.index ? mod.name + mod.index.toString() : mod.name;
        let name = '[' + modname + ']' + tab.name;

        if (tab.count !== undefined) {
          // clear to override existing values
          data.append(name, "");

          for (let value of formdata.getAll(name)) {
            // exclude empty values
            if (value && value != "")  {
              data.append(name, value);
            }
          }
        } else {
          let value = formdata.get(name);

          // explicitly set unchecked bool values to false
          if (tab.type == "bool" && !value) {
            value = "false";
          }

          data.append(name, value);
        }
      }
    }

    const response = await this.apiService.post('/api/config', data);
  }

  async upload(file) {
    const response = await this.apiService.upload('/config.ini', file, 'text/plain');
  }
}
