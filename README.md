# SesDönüştür — M4A'dan MP3'e (Linux)

SesDönüştür, Oğuz YILDIZ tarafından geliştirilen Qt tabanlı bir Linux masaüstü
uygulamasıdır. M4A, M4B, AAC ve MP4 dosyalarını MP3'e dönüştürür. Birden fazla
dosya ekleyebilir, dosyaları pencereye sürükleyip bırakabilir, dönüştürme
ilerlemesini izleyebilir ve işlemi iptal edebilirsiniz. Mevcut MP3 dosyalarının
üzerine yazılmaz.

Uygulama bilgileri ve sürüm notları için sağ üstteki **Bilgi** düğmesini kullanın.
İletişim: [oguzyildiz@live.com](mailto:oguzyildiz@live.com).

## Gereksinimler

- Qt 5 veya Qt 6 Widgets geliştirme araçları
- qmake
- C++17 derleyicisi
- FFmpeg (`ffmpeg`; süre bazlı ilerleme göstergesi için isteğe bağlı `ffprobe`)

Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential qtbase5-dev qt5-qmake ffmpeg
```

## Derleme ve çalıştırma

```bash
qmake m4a_to_mp3.pro
make
./m4a_to_mp3
```

Qt 6 kurulumlarında `qmake` yerine `qmake6` kullanın.

## Uygulama menüsü simgesi

Linux uygulama menüsüne kısayol eklerken
[`packaging/sesdonustur.desktop`](packaging/sesdonustur.desktop) dosyasını
`/usr/share/applications/` altına; [`assets/sesdonustur.svg`](assets/sesdonustur.svg)
dosyasını da
`/usr/share/icons/hicolor/scalable/apps/sesdonustur.svg` konumuna kurun.
Uygulama penceresinin simgesi programın içine çizildiğinden ayrıca bir Qt SVG
çalışma zamanı bileşeni gerekmez.

## Kullanım

1. Ses dosyalarını ekleyin veya pencereye sürükleyin.
2. MP3 dosyaları için bir çıkış klasörü seçin.
3. **Dönüştür** düğmesine basın.

FFmpeg, PATH üzerinden bulunmalıdır. Dönüştürülen dosyalar seçilen klasöre
`.mp3` uzantısıyla kaydedilir; aynı adlı dosya varsa mevcut dosya korunur ve
yeni çıktı benzersiz bir ad alır.
