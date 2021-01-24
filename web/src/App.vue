<style src="./styles/main.css"></style>
<style src="./styles/form.css"></style>
<style src="normalize.css"></style>
<style>

</style>

<template>
<div id="app" class="app">
  <header>
    <nav>
      <router-link to="/" active-class="">qmsk::esp</router-link>
      <router-link to="/config">Config</router-link>
    </nav>
  </header>
  <router-view> </router-view>
  <footer>

  </footer>
</div>
</template>

<script>
export default {
  data() {
    return {

    }
  }
}
</script>
