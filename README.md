* `v4l2-decode.c` is from https://gitlab.collabora.com/koike/v4l2-codec.git

* constantly keep dmesg logs: `tail -f /var/log/kern.log`

* `0001-dafna-prints.patch` debug prints patch

#### Video4linux2


I encountered problems compiling the v4l-utils program.
I use ubuntu 18.04, I tried to compile with qt4 but I got a compilation error:

```
qvidcap.cpp:14:10: fatal error: QtMath: No such file or directory
 #include <QtMath>
```

I think that QtMath is available only on qt5

So I installed qt5 with `sudo apt install qtbase5-dev` and configured and compiled again (after make clean),
This time I got compilation error in  moc_qv4l2.cpp, starting with the error:

```
moc_qv4l2.cpp:13:2: error: #error "This file was generated using the moc from 4.8.7. It"
 #error "This file was generated using the moc from 4.8.7. It"
  ^~~~~
```

This cpp file is generated with `moc`  and  `/usr/bin/moc` is a symlink to `/usr/bin/qtchooser` 

`qtchooser` use a conf file to decide which qt to use, apparently the conf file 
`/usr/lib/x86_64-linux-gnu/qt-default/qtchooser/default.conf`
is a symlink to `/usr/share/qtchooser/qt4-x86_64-linux-gnu.conf`,
So I just changed it:

```
sudo ln -s -T /usr/share/qtchooser/qt5-x86_64-linux-gnu.conf /usr/lib/x86_64-linux-gnu/qt-default/qtchooser/default.conf -f
```

==============
Uvcvideo is the module that creates the /dev/video0  /dev/video1, so removing it also removes those files
But it is needed to remove it in order to reinstall videodev since uvcvideo depends on it


========================

