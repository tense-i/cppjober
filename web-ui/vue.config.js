const { defineConfig } = require('@vue/cli-service')

module.exports = defineConfig({
  transpileDependencies: true,
  devServer: {
    port: 8081,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        pathRewrite: {
          '^/api': '/api'
        }
      }
    }
  },
  // 生产环境配置
  productionSourceMap: false,
  // 输出目录配置
  outputDir: '../web',
  // 静态资源目录
  assetsDir: 'static',
  // 公共路径
  publicPath: '/'
}) 