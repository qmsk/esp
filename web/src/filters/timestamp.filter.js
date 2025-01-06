export default function (date) {
  const now = new Date();

  if (!date) {
    return "";
  } else if (now.toDateString() == date.toDateString()) {
    return date.toLocaleTimeString();
  } else {
    return date.toLocaleString();
  }
}
