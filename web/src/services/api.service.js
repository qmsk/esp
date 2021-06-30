import axios from 'axios'

export default class APIService {
  get(url) {
    return axios.get(url);
  }

  post(url, data) {
    return axios.post(url, data);
  }

  upload(url, file, content_type) {
    return axios.post(url, file, {
      headers: {
        'Content-Type': content_type,
      },
    });
  }
}
