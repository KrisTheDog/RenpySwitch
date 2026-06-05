set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=7.6.3
export PYGAME_SDL2_VER=2.1.0
INITIAL_DIR=$(pwd)

apt-get -y update
apt-get -y upgrade

# Устанавливаем зависимости для сборки Python и OpenSSL 1.1
apt -y install build-essential checkinstall zlib1g-dev libncurses5-dev libncursesw5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev libnss3-dev libreadline-dev libffi-dev wget

# Устанавливаем зависимости для сборки хост-модулей Ren'Py
apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx

# =========================================================
# Скачиваем и собираем OpenSSL 1.1 (иначе pip2 не сможет скачивать пакеты)
# =========================================================
cd /tmp
wget https://www.openssl.org/source/openssl-1.1.1w.tar.gz
tar -xzvf openssl-1.1.1w.tar.gz
cd openssl-1.1.1w
./config --prefix=/usr/local/openssl-1.1 --openssldir=/usr/local/openssl-1.1 shared
make -j$(nproc)
make install
echo "/usr/local/openssl-1.1/lib" > /etc/ld.so.conf.d/openssl-1.1.conf
ldconfig

# =========================================================
# Скачиваем и собираем Python 2.7.18
# =========================================================
cd /tmp
wget https://www.python.org/ftp/python/2.7.18/Python-2.7.18.tgz
tar -xzf Python-2.7.18.tgz
cd Python-2.7.18

export CFLAGS="-I/usr/local/openssl-1.1/include"
export LDFLAGS="-L/usr/local/openssl-1.1/lib"
./configure --enable-optimizations --prefix=/usr/local
make -j$(nproc)
make altinstall

ln -sf /usr/local/bin/python2.7 /usr/local/bin/python2
ln -sf /usr/local/bin/pip2.7 /usr/local/bin/pip2

rm -rf /tmp/openssl-1.1.1w /tmp/Python-2.7.18

# Возвращаемся в рабочую папку!
cd $INITIAL_DIR

python2 --version

# Установка pip и зависимостей Python
curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
python2 get-pip.py
pip2 --version 

pip2 uninstall distribute -y || true
pip2 install future six typing requests ecdsa pefile==2019.4.18 Cython==0.29.36 setuptools==0.9.8

# =========================================================
# Настройка окружения Switch
# =========================================================
# Скачиваем ТОЛЬКО кастомный Python 2.7 для Switch. 
# Всё остальное (компилятор, SDL2, fribidi) УЖЕ есть в Docker-контейнере!
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/python27-switch.zip
unzip -qq python27-switch.zip -d $DEVKITPRO/portlibs/switch
rm python27-switch.zip

# =========================================================
# Скачиваем исходники Ren'Py
# =========================================================
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2

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

# Копируем кастомный subprocess.pyo (используем абсолютный путь от INITIAL_DIR)
cp -rf $INITIAL_DIR/subprocess.pyo renpy_sdk/renpy-$RENPY_VER-sdk/lib/python2.7

# =========================================================
# Патчи и подготовка исходников
# =========================================================
pushd renpy-source
patch -p1 < ../renpy.patch

# === Патчим ffmedia.c для совместимости с современным FFmpeg (API 5.1+) ===
# 1. Добавляем const в функцию записи
sed -i 's/static int rwops_write(void \*opaque, uint8_t \*buf/static int rwops_write(void *opaque, const uint8_t *buf/g' module/ffmedia.c

# 2. Заменяем устаревшее присвоение channel_layout на современный AVChannelLayout
sed -i 's/converted_frame->channel_layout = AV_CH_LAYOUT_STEREO;/converted_frame->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;/g' module/ffmedia.c

# 3. Заменяем проверку channel_layout на проверку порядка каналов
sed -i 's/!ms->audio_decode_frame->channel_layout/ms->audio_decode_frame->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC/g' module/ffmedia.c

# 4. Заменяем старую функцию получения дефолтного layout на новую
sed -i 's/ms->audio_decode_frame->channel_layout = av_get_default_channel_layout(ms->audio_decode_frame->channels);/av_channel_layout_default(\&ms->audio_decode_frame->ch_layout, ms->audio_decode_frame->ch_layout.nb_channels);/g' module/ffmedia.c

# 5. Заменяем обращение к channels на nb_channels
sed -i 's/ms->audio_decode_frame->channels == 1/ms->audio_decode_frame->ch_layout.nb_channels == 1/g' module/ffmedia.c

# 6. Обновляем вызов swr_alloc_set_opts на swr_alloc_set_opts2
sed -i 's/swr_alloc_set_opts(/swr_alloc_set_opts2(/g' module/ffmedia.c
sed -i 's/ms->swr = swr_alloc_set_opts2(ms->swr,/swr_alloc_set_opts2(\&ms->swr,/g' module/ffmedia.c

# 7. Передаем указатели на ch_layout вместо значений channel_layout
sed -i 's/converted_frame->channel_layout,/\&converted_frame->ch_layout,/g' module/ffmedia.c
sed -i 's/ms->audio_decode_frame->channel_layout,/\&ms->audio_decode_frame->ch_layout,/g' module/ffmedia.c
# ==========================================================================

pushd module
rm -rf gen gen-static
popd
popd

pushd pygame_sdl2-source
rm -rf gen gen-static
popd
