# Zuma-on-C-without-OS

## UEFI x64

Добавлена сборка игры как самостоятельного UEFI-приложения. Меню и игровой
код используются из `src`; Windows DLL для запуска не нужны. Эта версия
работает без звука, с программной отрисовкой через GOP.

Из PowerShell в папке проекта:

```powershell
.\uefi\build.ps1
```

Нужны Visual Studio 2022 с C++ x64 tools и заголовки EDK II. Скрипт сам находит
соседние `VisualUEFI\edk2` и `edk2-vscode-starter-v3\edk2-vscode-starter\third_party\edk2`.
Другой путь можно указать явно:

```powershell
.\uefi\build.ps1 -Edk2 C:\path\to\edk2
```

Результат — `build-uefi\esp\EFI\BOOT\BOOTX64.EFI` вместе с ресурсами.
Скопируйте **содержимое `build-uefi\esp` целиком** на FAT32-флешку, сохранив
папку `EFI`, и выберите её в UEFI Boot Menu. Для неподписанной сборки Secure
Boot должен быть выключен. Из UEFI Shell можно запустить
`fs0:\EFI\BOOT\BOOTX64.EFI` (номер `fs0` зависит от системы).

Мышь: левая кнопка — выстрел/выбор, правая — смена шара.
Без мыши: стрелки или WASD перемещают курсор, Enter/пробел нажимают левую
кнопку, X — правую. Esc завершает приложение и возвращает управление UEFI.

Подробности и запуск в эмуляторе: [uefi/README.md](uefi/README.md).

## Сборка для ОС

```
gcc -fcommon @sources.txt `pkg-config --cflags --libs sdl2 SDL2_image expat` -I ./include -L ./bin -lbass -lbass_fx -o bin/Zuma.exe -mwindows
```
Заходите через терминал в bin
```
cd bin
./Zuma.exe
```
Через двойной клик пока не оч работает
