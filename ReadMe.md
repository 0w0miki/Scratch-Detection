# 运行环境配置
## 安装Ubuntu16.04
### 更新软件源
```
sudo cp /etc/apt/sources.list /etc/apt/sources.list.bak
sudo gedit /etc/apt/sources.list
```
复制以下内容替换
```
# 默认注释了源码镜像以提高 apt update 速度，如有需要可自行取消注释 
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial main restricted universe multiverse 
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial main restricted universe multiverse 
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-updates main restricted universe multiverse 
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-updates main restricted universe multiverse 
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-backports main restricted universe multiverse 
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-backports main restricted universe multiverse 
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-security main restricted universe multiverse 
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-security main restricted universe multiverse 
# 预发布软件源，不建议启用 
# deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-proposed main restricted universe multiverse 
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ xenial-proposed main restricted universe multiverse
```
终端中运行
```
sudo apt-get update
sudo apt-get upgrade
```

### 安装ssh
```
sudo apt-get install openssh-server
```
* 查看是否安装成功:
    `dpkg -l | grep ssh`
    窗口中包含`openssh-server`则成功
#### ssh链接
* linux下 `ssh {name}@{ip}`, 密码: 安装系统ubuntu时设置的密码

## 安装cmake
```
sudo apt-get install cmake
```

## 安装opencv
环境安装
```
sudo apt-get install build-essential
sudo apt-get install pkg-config libgtk2.0-dev pkg-config libavresample-dev libavcodec-dev libavformat-dev libswscale-dev libjpeg-dev libpng-dev libtiff5-dev libjasper-dev libdc1394-22-dev
sudo apt-get install python-numpy libtbb2 libtbb-dev
```
解压编译
```
unzip opencv-3.4.3.zip
cd opencv-3.4.3
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

## 安装相机驱动
使用附带的驱动压缩包或者
在 http://www.daheng-imaging.com/Downloads/index.aspx?nodeid=279 下载USB3 Linux驱动 
压缩文件为`DAHENG_Camera_Software_Suite_for_GigE&USB3_Linux_x86.tar.gz`
在终端中输入
```
tar -zxvf DAHENG_Camera_Software_Suite_for_GigE&USB3_Linux_x86.tar.gz
cd dhcam_install_20180302
sudo ./dhmercam_install.bin
sudo bash ./SetUSBStack.sh
```

## 安装libcurl
`sudo apt-get install libcurl4-openssl-dev`

## smb服务
### 安装
```
sudo apt-get install samba
```
### 配置
* 修改配置文件
```
cd /etc/samba
sudo cp smb.conf smb.conf.bak
sudo vim smb.conf
```
在最后添加
```
[share]
comment = Share image folder
path = /home/{user}/gree/images    # samba登录的时候的路径，通俗说就是放东西的地方，这个路径创立的时候记得添加权限。
available = yes       # 下面就是一些权限的配置。 
valid user = {user}   # {user}替换为用户名
public = no
browseable = yes 
writable = yes
printable = no
create mask = 0765
```
### 启动服务
```
sudo /etc/init.d/samba restart
```

# 程序安装
复制压缩包detect.zip并解压
```
mkdir build
cd build
cmake ..
make
```
## 运行
参数位于setting.json文件中

mono为黑白摄像头程序（第一道检测），运行时在终端输入
```
cd ~/gree/build
sudo -E ./mono
```

color为彩色摄像头程序（第二道检测）
```
cd ~/gree/build
sudo -E ./color
```

图片存放在`~/gree/images`文件夹下， 其中`templates`文件夹存放原图模板，`test`文件夹存放拍摄到的图片。通过SMB服务将原图和拍摄图片取走。

## 设置开机启动
* 修改`/etc/rc.local`, 在**exit 0前**添加以下内容
``` shell
exec 2> /tmp/rc.local.log  # send stderr from rc.local to a log file  
exec 1>&2                  # send stdout to the same log file  
set -x                     # tell sh to display commands before execution

export GENICAM_CACHE_V2_3=/usr/local/daheng/genicam_xml_cache
export GENICAM_GENTL64_PATH=/usr/lib
export GENICAM_ROOT_V2_3=/home/换成用户名/dhcam_install_20181107/dh_camera/daheng-sdk-x64/sdk/genicam
export GENICAM_LOG_CONFIG_V2_3=$GENICAM_ROOT_V2_3/log/config-unix
cd /home/换成用户名/gree/build
sudo -E ./color
```

* 赋予`rc.local`执行权限
```
sudo chmod 777 /etc/rc.local
```

* 停止程序
```
systemctl stop rc-local.service
```

## 下载图片
### 赋予下载脚本执行权限
``` shell
cd ~/gree
chmod +x ./downloadpic.sh
```
### 添加定时任务
```
crontab -e
```
添加以下内容 每天0点10分执行复制工作
10 00 * * * * ~/gree/downloadpic.sh smb原图路径