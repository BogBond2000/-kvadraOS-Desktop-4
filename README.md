# Поиск media

Приложение на C++17 для Linux, которое периодически сканирует домашний каталог в поисках мультимедийных файлов (аудио, видео, изображения) и сохраняет результат в JSON-файл `~/.media_files`. Результат также доступен через HTTP.


## Сборка

```bash
git clone https://github.com/<username>/media_watcher.git
cd media_watcher
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Запуск

### Базовый запуск
Сканирует домашний каталог каждые 60 секунд:
```bash
./media_watcher
```

### С параметрами
```bash
./media_watcher --path /home/user/Music --interval 30
```

### Параметры

| Флаг | Краткий | По умолчанию | Описание |
|---|---|---|---|
| `--path` | `-p` | `~` | Каталог для сканирования |
| `--interval` | `-i` | `60` | Интервал сканирования в секундах |

---

## Результат

### Файл `~/.media_files`
После каждого сканирования создаётся или обновляется файл:
```bash
cat ~/.media_files
```

```json
{
    "audio": [
        "111.mp3",
        "222.wav"
    ],
    "video": [
        "333.mpg"
    ],
    "images": [
        "444.jpeg",
        "555.png"
    ]
}
```

### HTTP
Результат также доступен через HTTP:
```bash
curl http://localhost:1234/media_files
```

---

## Поддерживаемые форматы

## Поддерживаемые форматы

| Категория | Расширения |
|---|---|
| Аудио | `mp3` `wav` `flac` `ogg` `aac` `m4a` `wma` `opus` `aiff` `aif` `ape` `wv` `mka` `ra` `ram` `mid` `midi` `amr` `dts` `ac3` |
| Видео | `mp4` `mpg` `mpeg` `avi` `mkv` `mov` `webm` `wmv` `flv` `f4v` `m4v` `3gp` `3g2` `ogv` `ts` `m2ts` `mts` `vob` `rm` `rmvb` `divx` `asf` |
| Изображения | `jpg` `jpeg` `png` `gif` `bmp` `webp` `tiff` `tif` `svg` `ico` `heic` `heif` `raw` `cr2` `nef` `arw` `dng` `psd` `xcf` `avif` `jxl` `pbm` `pgm` `ppm` |

---

## Остановка

```bash
Ctrl+C
```

---

## Тесты

```bash
cd build
ctest --output-on-failure
```

---

## Структура проекта

```
media_watcher/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp                  # Точка входа, цикл сканирования
    ├── scanner.hpp / .cpp        # Обход каталога и классификация файлов
    ├── json_builder.hpp / .cpp   # Формирование JSON
    ├── http_server.hpp / .cpp    # HTTP-сервер на POSIX-сокетах
    └── fd_handle.hpp             # RAII-обёртка для файловых дескрипторов
```

