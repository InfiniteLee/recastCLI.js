const path = require("path");
const HTMLWebpackPlugin = require("html-webpack-plugin");

module.exports = {
  entry: path.join(__dirname, "index.js"),
  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "bundle.js"
  },
  module: {
    rules: [
      {
        test: /\.worker\.js$/,
        loader: "worker-loader"
      },
      { 
        test: /\.wasm$/,
        type: "javascript/auto",
        loader: "file-loader"
      }
    ]
  },
  plugins: [
    new HTMLWebpackPlugin({
      template: path.join(__dirname, "index.html")
    })
  ],
  target: 'web',
  node: {
    __dirname: false,
    fs: 'empty',
    Buffer: false,
    process: false
  }
};
