import axios from 'axios'

class APIError extends Error {
  static fromAxios(error) {
    let message = `HTTP ${error.config.method} ${error.config.url} => HTTP ${error.response.status} ${error.response.statusText}`;
    let text = null;
    let data = null;

    if (error.response.headers['content-type'] == 'text/plain') {
      text = error.response.data;
      message = `${message}\n${text}`;
    } else if (error.response.headers['content-type'] == 'application/json') {
      data = error.response.data;
    }

    return new APIError(
      error.config.method,
      error.config.url,
      error.response.status,
      error.response.statusText,
      text, data,
      message
    );
  }

  constructor(method, url, status, statusText, text, data, message) {
    super(message);

    this.name = "APIError";

    this.method = method;
    this.url = url;
    this.status = status;
    this.statusText = statusText;
    this.text = text;
    this.data = data;
  }
}

export default class APIService {
  async get(url, params = {}) {
    try {
      return await axios.get(url, {params});
    } catch (error) {
      if (error.response) {
        throw APIError.fromAxios(error);
      } else {
        throw error;
      }
    }
  }

  async post(url, data) {
    try {
      return await axios.post(url, data);
    } catch (error) {
      if (error.response) {
        throw APIError.fromAxios(error);
      } else {
        throw error;
      }
    }
  }

  async delete(url) {
    try {
      return await axios.delete(url);
    } catch (error) {
      if (error.response) {
        throw APIError.fromAxios(error);
      } else {
        throw error;
      }
    }
  }

  async upload(url, file, content_type) {
    try {
      return await axios.post(url, file, {
        headers: {
          'Content-Type': content_type,
        },
      });
    } catch (error) {
      if (error.response) {
        throw APIError.fromAxios(error);
      } else {
        throw error;
      }
    }
  }

  async putFile(url, file, progress) {
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

    try {
      return await axios.put(url, file, config);
    } catch (error) {
      if (error.response) {
        throw APIError.fromAxios(error);
      } else {
        throw error;
      }
    }
  }
}
