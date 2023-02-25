export default class VFSService {
  constructor(apiService) {
    this.apiService = apiService;
  }

  async get() {
    const response = await this.apiService.get('/vfs');

    return response.data
  }
}
