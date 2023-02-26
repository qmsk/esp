import axios from 'axios'

export default class APIService {
  get(url) {
    return axios.get(url);
  }

  post(url, data) {
    return axios.post(url, data);
  }

  delete(url) {
    return axios.delete(url);
  }

  upload(url, file, content_type) {
    return axios.post(url, file, {
      headers: {
        'Content-Type': content_type,
      },
    });
  }

  putFile(url, file,) {
    return axios.put(url, file, {
      headers: {
        'Content-Type': file.type || 'application/binary',
        'Last-Modified': file.lastModified,
      },
    });
  }
}
