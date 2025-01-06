export default class UserService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async getStatus() {
    const response = await this.apiService.get('/api/status');

    return response.data
  }
  
  async postButton(button, event) {
    let data = new URLSearchParams();

    data.append(button, event);

    const response = await this.apiService.post('/api/button', data);

    return response.data
  }

}
