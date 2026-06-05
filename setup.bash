set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=7.6.3
export PYGAME_SDL2_VER=2.1.0
INITIAL_DIR=$(pwd)
apt-get -y update
apt-get -y upgrade

apt -y install build-essential checkinstall
apt -y install libncursesw5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev

#apt -y install python2 python2-dev
#python2 --version
#curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
#python2 get-pip.py
#pip2 --version 




# Устанавливаем зависимости для сборки Python и OpenSSL 1.1
apt -y install build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev libsqlite3-dev wget libbz2-dev

# Скачиваем и собираем OpenSSL 1.1 (иначе pip2 не сможет скачивать пакеты из-за несовместимости с OpenSSL 3.x)
cd /tmp
wget https://www.openssl.org/source/openssl-1.1.1w.tar.gz
tar -xzvf openssl-1.1.1w.tar.gz
cd openssl-1.1.1w
./config --prefix=/usr/local/openssl-1.1 --openssldir=/usr/local/openssl-1.1 shared
make -j$(nproc)
make install
echo "/usr/local/openssl-1.1/lib" > /etc/ld.so.conf.d/openssl-1.1.conf
ldconfig

# Скачиваем и собираем Python 2.7.18 (последняя версия Python 2)
cd /tmp
wget https://www.python.org/ftp/python/2.7.18/Python-2.7.18.tgz
tar -xzf Python-2.7.18.tgz
cd Python-2.7.18

# Указываем пути к собранному OpenSSL 1.1
export CFLAGS="-I/usr/local/openssl-1.1/include"
export LDFLAGS="-L/usr/local/openssl-1.1/lib"
./configure --enable-optimizations --prefix=/usr/local
make -j$(nproc)
make altinstall

# Создаем симлинки, чтобы скрипт видел команды python2 и pip2
ln -sf /usr/local/bin/python2.7 /usr/local/bin/python2
ln -sf /usr/local/bin/pip2.7 /usr/local/bin/pip2

# Очищаем временные файлы сборки
rm -rf /tmp/openssl-1.1.1w /tmp/Python-2.7.18

cd $INITIAL_DIR

# Проверка
python2 --version

# Установка pip
curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
python2 get-pip.py
pip2 --version 







apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev  libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx
pip2 uninstall distribute
pip2 install future six typing requests ecdsa pefile==2019.4.18 Cython==0.29.36 setuptools==0.9.8

#curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/devkitpro-pkgbuild-helpers-2.2.4-2-any.pkg.tar.xz
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/python27-switch.zip
#curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/switch-libfribidi-1.0.12-1-any.pkg.tar.xz

# Удаляем строку установки старого пакета
# dkp-pacman -U --noconfirm devkitpro-pkgbuild-helpers-2.2.4-2-any.pkg.tar.xz
#dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
unzip -qq python27-switch.zip -d $DEVKITPRO/portlibs/switch

#rm devkitpro-pkgbuild-helpers-2.2.4-2-any.pkg.tar.xz
#rm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
rm python27-switch.zip

/bin/bash -c 'sed -i'"'"'.bak'"'"' '"'"'s/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /'"'"' $DEVKITPRO/switch.cmake'


curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2
#curl -LOC - https://www.renpy.org/dl/$RENPY_VER/android-native-symbols.zip
#curl -LOC - https://dl.otorh.in/github/rawproject.zip

rm -rf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
tar -xf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
rm pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

rm -rf renpy-$RENPY_VER-source renpy-source
tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-$RENPY_VER-source.tar.bz2

rm -rf renpy-$RENPY_VER-sdk renpy_sdk
unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm renpy-$RENPY_VER-sdk.zip
cp -rf subprocess.pyo renpy_sdk/renpy-$RENPY_VER-sdk/lib/python2.7

#dkp-pacman --noconfirm -S switch-libfribidi

#rm -rf raw
#unzip -qq rawproject.zip -d raw
#rm rawproject.zip

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
