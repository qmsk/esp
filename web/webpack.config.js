const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const VueLoaderPlugin = require('vue-loader/lib/plugin')
const MiniCssExtractPlugin = require('mini-css-extract-plugin');

module.exports = (env, argv) => {
  return {
    entry: './src/index.js',
    resolve: {
      alias: {
        'vue$': 'vue/dist/vue.esm.js', // full bundle with compiler
      },
      extensions: ['.js', '.vue'],
    },
    module: {
      rules: [
        {
          test: /\.vue$/,
          loader: 'vue-loader',
        },
        {
          test: /\.css$/,
          use: [
            argv.mode !== 'production' ? 'vue-style-loader' : MiniCssExtractPlugin.loader,
            'css-loader'
          ],
        },
      ],
    },
    plugins: [
      new VueLoaderPlugin(),
      new MiniCssExtractPlugin(),
      new HtmlWebpackPlugin({
        template: './src/index.html',
      }),
    ],
    output: {
      filename: 'main.js',
      path: path.resolve(__dirname, 'dist'),
      publicPath: '/dist/',
    },
    devServer: {
      contentBase: './dist',
      proxy: {
        '/api': process.env.API_URL,
        '/config.ini': process.env.API_URL,
        '/vfs': process.env.API_URL,
      }
    },
  };
};
