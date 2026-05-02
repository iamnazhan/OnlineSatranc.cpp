# C++ Windows Online Satranç

Bu sürüm Windows odaklı hazırlanmıştır.

## Eklenen / düzeltilenler

- Windows istemci: `ChessWin.exe`
- Windows sunucu: `ChessServerWin.exe`
- Giriş ekranı: daha estetik kart tasarımı, yeni buton stilleri ve geliştirilmiş metinler
- Oyun seçenekleri: bota karşı, 2 oyuncu aynı PC, online mod
- Taş seti: gönderdiğin örneğe göre özel görsel taş seti eklendi (`assets/pieces`)
- Online bağlantıda arayüz donmasın diye bağlantı işlemi ayrı thread içinde ve 6 saniye timeout ile yapıldı
- Online sunucu artık Linux zorunlu değil, Windows üzerinde çalışır
- Farklı ağ için TCP port yönlendirme/VPS kullanımına uygun hale getirildi

> Not: Hesaplar basit proje ödevi mantığıyla exe yanındaki `accounts.txt` dosyasında tutulur. Gerçek güvenlik için veritabanı + şifre hash sistemi gerekir.

## Derleme düzeltmesi

Bu pakette Windows SDK / GDI+ için gerekli `objidl.h`, `propidl.h` ve `std::max` düzeltmeleri yapılmıştır.

## Windows derleme

Gerekenler:

- Visual Studio 2022 veya Visual Studio Build Tools
- CMake

Komutlar:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Çıkan dosyalar:

```bat
build\Release\ChessWin.exe
build\Release\ChessServerWin.exe
```

## Aynı bilgisayarda online test

1. `build\Release\ChessServerWin.exe` çalıştır.
2. İki tane `ChessWin.exe` aç.
3. İkisinde de hesap aç / giriş yap.
4. IP alanına `127.0.0.1`, port alanına `5555` yaz.
5. İki istemcide de **Online Bağlan** de.

İlk bağlanan beyaz, ikinci bağlanan siyah olur.

## Aynı ağda oynama

Sunucu olacak bilgisayarda:

```bat
ipconfig
```

IPv4 adresini bul. Örnek:

```txt
192.168.1.35
```

Diğer bilgisayarların istemcisinde IP kısmına bu adresi yaz:

```txt
192.168.1.35
```

Port:

```txt
5555
```

Windows Defender Firewall sorarsa `Allow access / İzin ver` de.

## Farklı ağlarda oynama

Farklı ağlarda çalışması için istemcilerin sunucu bilgisayara internetten ulaşabilmesi gerekir. Bunun 2 yolu var:

### Yöntem 1: Port yönlendirme

Sunucu bilgisayarın modem/router panelinde TCP `5555` portunu sunucu bilgisayarın yerel IP adresine yönlendir.

Örnek:

```txt
Dış TCP port: 5555
İç IP: 192.168.1.35
İç TCP port: 5555
```

Sonra diğer ağdaki oyuncular IP alanına sunucu tarafının dış IP adresini yazar.

### Yöntem 2: VPS / Windows Server

Bir Windows VPS kiralayıp projeyi orada çalıştırırsan herkes VPS IP adresine bağlanabilir. Bu yöntem daha sağlamdır.

## Not

`ChessWin.exe` derlendikten sonra CMake otomatik olarak `assets` klasörünü exe yanına kopyalar. Böylece yeni taş görselleri doğrudan yüklenir.

## Hızlı çalıştırma BAT dosyaları

Derledikten sonra şunları kullanabilirsin:

```bat
run_server_windows.bat
run_client_windows.bat
```

## Dosya yapısı

```txt
online_chess_cpp/
├── CMakeLists.txt
├── README.md
├── run_client_windows.bat
├── run_server_windows.bat
└── src/
    ├── common/
    │   ├── Chess.h
    │   └── Chess.cpp
    ├── client/
    │   └── WinClient.cpp
    └── server/
        ├── WindowsTcpServer.cpp
        └── LinuxTcpServer.cpp
```
