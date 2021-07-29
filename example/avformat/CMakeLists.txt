#source
AUX_SOURCE_DIRECTORY(. DIR_LIB_SRCS)
#生成静态库
ADD_LIBRARY(avformat_example ${DIR_LIB_SRCS})
