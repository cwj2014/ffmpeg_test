#source
AUX_SOURCE_DIRECTORY(. DIR_LIB_SRCS)
#生成静态库
ADD_LIBRARY(avutil_example ${DIR_LIB_SRCS})
