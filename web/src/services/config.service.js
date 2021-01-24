export default class ConfigService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/api/config');

    return response.data
  }
}
