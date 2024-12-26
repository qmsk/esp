export default class ArtNetService {
    constructor(apiService) {
      this.apiService = apiService;
    }
  
    async get() {
      const response = await this.apiService.get('/api/artnet');
  
      return response.data
    }
  }
  