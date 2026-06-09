
// normalize config API value from HTML formdata based on configtab type
function normalizeFormValue(type, value) {
  if (type == 'color') {
    return value.substring(1);
  } else if (type == 'bool' && !value) {
    // explicitly set unchecked bool values to false
    return 'false';
  } else {
    return value;
  }
}

// normalize config API value from HTML formdata based on configtab type
function normalizeFormValues(type, values) {
  if (type == 'color') {
    return values.map((v) => normalizeFormValue(type, v));
  } else {
    // exclude empty values
    return values.filter((v) => v && v != "");
  }
}
  
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

        if (tab.migrated) {
          // skip
        } else if (tab.count !== undefined) {
          // clear to override existing values
          data.append(name, "");

          for (let value of normalizeFormValues(tab.type, formdata.getAll(name))) {
            data.append(name, value);
          }
        } else {
          let value = normalizeFormValue(tab.type, formdata.get(name));

          data.append(name, value);
        }
      }
    }

    const response = await this.apiService.post('/api/config', data);

    return response.data;
  }

  async upload(file) {
    const response = await this.apiService.upload('/config.ini', file, 'text/plain');
  }
}
