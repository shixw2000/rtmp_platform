# RTMP Platform for for multimedia #
# 支持RTMP协议的多媒体平台架构 #

## 综述 ##
 本项目实现了一个支持RTMP协议的平台接入功能。对于视频发布者，或者视频播放者，可按照指定路径（例如：rtmp://192.168.1.200/shixw/livestream)，实现实时音视频交换。
 * 编译仅依赖一个openss1.1.1q版本的外部库[libcrypto.a]，实现对handshake的支持。