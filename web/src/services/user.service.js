export default class UserService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async getStatus() {
    const response = await this.apiService.get('/api/status');

    return response.data
  }
}
