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



### Установка QEMU на Windows

Выполните в PowerShell:

```powershell
winget install --id SoftwareFreedomConservancy.QEMU --exact --source winget
```

После установки проверьте наличие QEMU и прошивок UEFI:

```powershell
Test-Path "C:\Program Files\qemu\qemu-system-x86_64.exe"
Get-ChildItem "C:\Program Files\qemu\share" -Filter "*.fd"
```

Первая команда должна вывести `True`. Среди файлов прошивки должны
быть `edk2-x86_64-code.fd` и `edk2-i386-vars.fd`.

Разместите проект по пути без кириллицы, например
`C:\Projects\Zuma-on-C-without-OS`, чтобы избежать ошибок открытия
образа в QEMU.


### Как запускать?

Откройте PowerShell в корне проекта. После успешной сборки выполните:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\uefi\run-qemu.ps1
```

При стандартной установке скрипт автоматически найдёт QEMU и прошивку
UEFI, создаст образ игры и откроет виртуальную машину.

После изменения кода и повторной сборки закройте QEMU и обновите образ:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\uefi\run-qemu.ps1 -RefreshImage
```

**`-RefreshImage` удаляет сохранения внутри старого образа.**
Для последующих запусков используйте команду без этого параметра.

### Если игра работает очень медленно:


1. В Диспетчере задач откройте «Производительность → ЦП» 
   убедитесь, что «Виртуализация» включена. Если она отключена,
   включите её в настройках BIOS/UEFI компьютера.
2. Нажмите `Win + R`, введите `optionalfeatures` и нажмите Enter.
3. Включите компонент «Платформа низкоуровневой оболочки Windows»
   (Windows Hypervisor Platform).
4. Примените изменения и перезагрузите компьютер.

### Описание управления и чита на смену цвета шаров:

- Стрелки или WASD — перемещение прицела.
- Пробел или Enter — выстрел / выбор.
- X — смена шара.
- Цифры 1–6 над буквами — чит выбора цвета заряженного шара.
  Доступное число цветов зависит от сложности.
- Esc — выход из приложения.

В UEFI-версии звук отключён. Если мышь недоступна, используйте клавиатуру.

