# Zuma-on-C-without-OS
```
gcc -fcommon @sources.txt `pkg-config --cflags --libs sdl2 SDL2_image expat` -I ./include -L ./bin -lbass -lbass_fx -o bin/Zuma.exe -mwindows
```
Заходите через терминал в bin
```
cd bin
./Zuma.exe
```
Через двойной клик пока не оч работает
