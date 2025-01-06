export default class ArtNetService {
    constructor(apiService) {
      this.apiService = apiService;
    }
  
    async get() {
      const response = await this.apiService.get('/api/artnet');
  
      return response.data
    }

    async getInputs() {
      const response = await this.apiService.get('/api/artnet/inputs');
  
      return response.data
    }
    async getOutputs() {
      const response = await this.apiService.get('/api/artnet/outputs');
  
      return response.data
    }
  }
  