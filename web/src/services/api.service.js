import axios from 'axios'

export default class APIService {
  get(url, params = {}) {
    return axios.get(url, {params});
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

  putFile(url, file, progress) {
    const lastModified = new Date(file.lastModified);

    let config = {
      headers: {
        'Content-Type': file.type || 'application/binary',
        'Last-Modified': lastModified.toUTCString(),
      },
    };

    if (progress) {
      config.onUploadProgress = function(progressEvent) {
        console.log("putFile " + url + ": progress loaded=" + progressEvent.loaded + " total=" + progressEvent.total);

        progress.loaded = progressEvent.loaded;
        progress.total = progressEvent.total;
      }
    }

    return axios.put(url, file, config);
  }
}
