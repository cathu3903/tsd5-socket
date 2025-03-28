# 五子棋游戏程序
## 2025-3-15 Version 0.1 使用 libsdl2 进行棋盘渲染 

> 要求使用C-S结构socket，并基于linux

该版本下需要安装 libsdl2-dev
```
sudo apt install libsdl2-dev
```

并安装sdl2 字体libsdl2-ttf-dev
```
sudo apt install libsdl2-ttf-dev
```

移动到exemple目录下
```
cd exemple
```

使用 makefile 进行编译
```
make
```

打开第一个终端运行服务器端
```
./serverur_TCP 8080 #端口号随意指定
```
打开另外两个客户端分别运行客户端
```
./client_TCP localhost 8080 #保持和client同一ip地址与端口号
```

## Version 0.2 计划使用静态图片取代sdl2渲染
1. 使用AI生成的静态图片取代sdl2渲染的棋盘与棋子 (hr)
2. 使用老师提供的tictactoe.h替代原本代码中的对应结构，并修改注释为英文(yn) 

## Version 0.2.2 结束游戏后玩家可以投票是否重新开始，并对每局游戏赢家计分，在投票重新开始界面可以看到两个玩家的分数
1. 使用循环结构()
2. 增加投票环节()
3. 增加计分变量并显示()
