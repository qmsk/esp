export default class WiFiService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/api/wifi');

    return response.data
  }
}
