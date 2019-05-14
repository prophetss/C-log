# C-log
   Easy and Efficient，Multiple threads And Multiple Handles.Support both console printing and file output.
   It offers Logging Levels、IO Buffer、 Backup、LOG Size Control、Stack print、Encrytion(AES-128)、Compression(lz4)、HashTab(MD5).
   
   这是一个简单、高效和轻量级C语言写的的日志系统，linux下不需要第三方库安装。目前主要是在linux下编写和测试，通用分支为一个基于Apache的APR(一个C语言跨平台的开源库)编写的跨平台版本，由于精力有限只基于最简单的日志打印功能进行了改写，仅供参考。
   日志系统支持多线程多句柄,同时支持控制台输出和文件输出。其他功能有控制台字体颜色控制样式背景色控制、印级别控制、IO缓存设置、备份控制、日志文件大小控制、异常退出堆栈打印、加密(AES-128)、压缩(lz4)和散列校验(MD5)。所有配置和接口都在log.h内。堆栈打印是接收所有异常退出信号时打印堆栈，输出文件名通过TRACE_PRINT_PATH宏控制，在其信号处理时还对所有日志句柄进行刷新处理，减少日志丢失可能。
   
   sample文件内有一个基本包含所有内容的使用样例，可参考使用。默认编译release版本，debug版本编译命令：make ver=debug。
