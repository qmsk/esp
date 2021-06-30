export default class ConfigService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/api/config');

    return response.data
  }

  async upload(file) {
    const response = await this.apiService.upload('/config.ini', file, 'text/plain');
  }
}
