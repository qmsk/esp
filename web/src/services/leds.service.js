export default class LedsService {
    constructor(apiService) {
      this.apiService = apiService;
    }
  
    async get() {
      const response = await this.apiService.get('/api/leds');

      return new Map(Object.entries(response.data));
    }
  }
  