set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=7.6.1
export PYGAME_SDL2_VER=2.1.0

apt-get -y update
apt-get -y upgrade

apt -y install build-essential checkinstall
apt -y install libncursesw5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev

apt -y install python2 python2-dev

python2 --version

curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
python2 get-pip.py
pip2 --version 

apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev  libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx
pip2 uninstall distribute
pip2 install future six typing requests ecdsa pefile==2019.4.18 Cython==0.29.36 setuptools==0.9.8

#apt-get -y install python2.7 python-pip p7zip-full cython libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev  libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx
#pip2 install future six typing requests ecdsa pefile==2019.4.18 Cython==0.29.36

curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/python27-switch.tar.gz
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/switch-libfribidi-1.0.12-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
tar -xf python27-switch.tar.gz -C $DEVKITPRO/portlibs/switch

rm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
rm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
rm python27-switch.tar.gz




mkdir -p switch-ffmpeg
pushd switch-ffmpeg
curl -LO https://github.com/greg6821-debug/scripts/releases/download/test/ffmpeg-6.0.zip
unzip -qq ffmpeg-6.0.zip -d .
# Скачиваем исходники FFmpeg
curl -LO https://ffmpeg.org/releases/ffmpeg-6.0.tar.xz
# Распаковываем исходники
tar -xf ffmpeg-6.0.tar.xz
# --- Сборка и установка switch-ffmpeg ---
# Инициализируем переменные DevkitPro для сборки
source $DEVKITPRO/switchvars.sh

# Создаём директорию для сборки
mkdir -p build

# Собираем пакет с помощью makepkg
# Если makepkg не установлен, используем стандартную сборку через PKGBUILD вручную
# 1) Применяем патчи
pushd ffmpeg-6.0
patch -Np1 -i ../ffmpeg-6.0.patch

# 2) Конфигурация сборки под Switch
  ./configure --prefix=$PORTLIBS_PREFIX --enable-gpl --disable-shared --enable-static \
    --cross-prefix=aarch64-none-elf- --enable-cross-compile \
    --arch=aarch64 --cpu=cortex-a57 --target-os=horizon --enable-pic \
    --extra-cflags='-D__SWITCH__ -D_GNU_SOURCE -O2 -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec' \
    --extra-cxxflags='-D__SWITCH__ -D_GNU_SOURCE -O2 -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec' \
    --extra-ldflags='-fPIE -L${PORTLIBS_PREFIX}/lib -L${DEVKITPRO}/libnx/lib' \
    --disable-runtime-cpudetect --disable-programs --disable-debug --disable-doc --disable-autodetect \
    --enable-asm --enable-neon \
    --disable-avdevice --disable-encoders --disable-muxers \
    --enable-swscale --enable-swresample --enable-network  \
    --disable-protocols --enable-protocol=file,http,ftp,tcp,udp,rtmp \
    --enable-zlib --enable-bzlib --enable-libass --enable-libfreetype --enable-libfribidi --enable-libdav1d \
    --enable-tx1
# 3) Собираем статически
make -j$(nproc)
# 4) Устанавливаем библиотеки в portlibs
make install
popd
# Очистка временных файлов
rm -rf switch-ffmpeg/build
popd







/bin/bash -c 'sed -i'"'"'.bak'"'"' '"'"'s/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /'"'"' $DEVKITPRO/switch.cmake'


curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/android-native-symbols.zip
#curl -LOC - https://dl.otorh.in/github/rawproject.zip
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/rawproject.zip

rm -rf pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER pygame_sdl2-source
tar -xf pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER pygame_sdl2-source
rm pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER.tar.gz

rm -rf renpy-$RENPY_VER-source renpy-source
tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-$RENPY_VER-source.tar.bz2

rm -rf renpy-$RENPY_VER-sdk renpy_sdk
unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm renpy-$RENPY_VER-sdk.zip

rm -rf raw
unzip -qq rawproject.zip -d raw
rm rawproject.zip

#mkdir -p ./raw/android/lib
#rm -rf android-native-symbols renpy_androidlib ./raw/android/lib
#unzip -qq android-native-symbols.zip -d ./raw/android/lib
#rm -rf ./raw/android/lib/x86_64/
#rm android-native-symbols.zip

pushd renpy-source
patch -p1 < ../renpy.patch
pushd module
rm -rf gen gen-static
popd
popd
pushd pygame_sdl2-source
rm -rf gen gen-static
popd
