export default class LedsService {
    constructor(apiService) {
      this.apiService = apiService;
    }
  
    async get() {
      const response = await this.apiService.get('/api/leds');

      return new Map(Object.entries(response.data));
    }
    async getStatus(leds) {
      const response = await this.apiService.get('/api/leds/status', { leds: leds });

      return response.data;
    }
  }
  