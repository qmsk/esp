import axios from 'axios'

export default class APIService {
  get(url) {
    return axios.get(url);
  }
}
