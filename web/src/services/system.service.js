export default class SystemService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async restart() {
    const response = await this.apiService.post('/api/system/restart');
  }
}
