export default class SystemService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/api/system');

    return response.data;
  }

  async restart() {
    const response = await this.apiService.post('/api/system/restart');
  }
}
