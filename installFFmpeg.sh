#!/bin/sh

set -e

ROOT=`pwd`
INSTALL_DIR=$ROOT/libdeps_install
LIBDEPS_SOURCE=$ROOT/libdeps
mkdir ${LIBDEPS_SOURCE}

#install hg: sudo apt-get install mercurial
#install numa: apt-get install -y libnuma-dev


install_x264(){
   echo "-------------begin install x264---------------"
   cd ${LIBDEPS_SOURCE}
   dir=x264
   url=https://code.videolan.org/videolan/x264.git
   if [ ! -d $dir ]; then
     git clone ${url} ${dir}
   fi
   cd ${dir}
   ./configure --prefix=${INSTALL_DIR} --disable-asm --enable-shared --bit-depth=all --enable-pic
   make -j4
   make install
}

install_x265(){
   echo "-------------begin install x265---------------"
   cd ${LIBDEPS_SOURCE}
   dir=x265
   url=http://hg.videolan.org/x265
   if [ ! -d $dir ]; then
     hg clone ${url} ${dir}
   fi
   cd ${dir}/build/linux
   #参考 x265/build/linux/multilib.sh
   mkdir -p 8bit 10bit 12bit
   cd 12bit
   cmake ../../../source -DHIGH_BIT_DEPTH=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DMAIN12=ON
   make ${MAKEFLAGS}

   cd ../10bit
   cmake ../../../source -DHIGH_BIT_DEPTH=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF
   make ${MAKEFLAGS}

   cd ../8bit
   ln -sf ../10bit/libx265.a libx265_main10.a
   ln -sf ../12bit/libx265.a libx265_main12.a
   cmake ../../../source -DEXTRA_LIB="x265_main10.a;x265_main12.a" -DEXTRA_LINK_FLAGS=-L. -DLINKED_10BIT=ON -DLINKED_12BIT=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
   make ${MAKEFLAGS}

   # rename the 8bit library, then combine all three into libx265.a

   mv libx265.a libx265_main.a
   uname=`uname`

   if [ "$uname" = "Linux" ]; then
    # On Linux, we use GNU ar to combine the static libraries together
ar -M <<EOF
    CREATE libx265.a
    ADDLIB libx265_main.a
    ADDLIB libx265_main10.a
    ADDLIB libx265_main12.a
    SAVE
    END

EOF

   else
    # Mac/BSD libtool
    libtool -static -o libx265.a libx265_main.a libx265_main10.a libx265_main12.a 2>/dev/null
   fi
  make install
}

install_FFmpeg(){
   echo "-------------begin install ffmpeg---------------"
   cd ${LIBDEPS_SOURCE}
   dir=ffmepg
   url=https://git.ffmpeg.org/ffmpeg.git
   if [ ! -d $dir ]; then
     git clone ${url} ${dir}
   fi
   cd ${dir}
   echo "install ${INSTALL_DIR}"
   export PKG_CONFIG_PATH=${INSTALL_DIR}/lib/pkgconfig:$PKG_CONFIG_PATH
   ./configure --prefix=${INSTALL_DIR} \
               --enable-gpl \
               --enable-version3 \
               --enable-nonfree \
               --enable-libx264 \
               --enable-libx265 \
               --enable-pic \
               --disable-asm \
               --extra-cflags=-I${INSTALL_DIR}/include \
               --extra-ldflags=-L${INSTALL_DIR}/lib \
               --extra-libs='-lstdc++ -lm -lrt -ldl -lpthread'
   make -j4
   make install         
}

install_x264
install_x265
install_FFmpeg
