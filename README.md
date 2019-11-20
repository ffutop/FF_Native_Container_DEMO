# 一个简单的容器化 DEMO

demo 在原生操作系统上直接开始操作。不借助 docker 等高度集成的容器化工具，仅使用常用的 `mount`, `iptables` 等命令。意图使大家对容器化有一个较为直观的认识。

*如有不合理之处，烦请私信指正:)*

## 挂载一个联合文件系统

容器化一个重要的特征在于：提供了不可变的基础镜像层，以及容器运行时可写的容器层。所有对文件的增删改操作都将实际发生在容器层，而持久保证镜像层的不变。

1. 下载一个基础的根文件系统

```bash
mkdir -p /mnt/rootfs/lower
cd /mnt/rootfs/lower
curl -LO http://dl-cdn.alpinelinux.org/alpine/v3.10/releases/x86_64/alpine-minirootfs-3.10.3-x86_64.tar.gz 
tar -xvf alpine-minirootfs-3.10.3-x86_64.tar.gz
```

简单的观察一下解压的内容，下载的是一个最小的文件系统，`/proc`, `/root`, `/etc` 等目录一应俱全。我们将以解压在 `/mnt/rootfs/lower` 目录下的所有内容作为镜像层。

2. 创建一个容器层

```bash
mkdir -p /mnt/rootfs/upper
```

3. 挂载一个联合文件系统

```bash
mkdir -p /mnt/rootfs/work
mkdir -p /mnt/rootfs/merge
cd /mnt/rootfs
mount -t overlay overlayfs -o lowerdir=lower,upperdir=upper,workdir=work merge/
```

可以观察一下 `/mnt/rootfs/merge` 目录，创建的空目录，因为 `mount` 操作，展示的结果和 `/mnt/rootfs/lower` 目录一模一样了。

在 `/mnt/rootfs/merge` 目录下对文件进行增删改操作，可以观察下 `/mnt/rootfs/lower` 和 `/mnt/rootfs/upper` 目录，`lower` 目录保持不变，但 `upper` 目录因为 `merge` 目录下的文件操作，做出了相应的变更。

## 编译执行实施容器隔离的进程

```bash
# compile
make fns
# run
./fns 
# change root
chroot /mnt/rootfs/merge /bin/sh
# mount /proc directory
mount -t proc proc /proc
```

`fns.c` 程序已经将 **mount**, **uts**, **pid**, **net**, **ipc** 等命名空间隔离。

借助 `ps`, `netstat`, `mount` 等命令，可以观察到当前进程（及其子进程）与其它主机进程的有极大的不同。

## 打通容器网络与主机网络

```sh
# create a pair of virtual Ethernet
ip link add type veth

# move veth1 to container net ns (host net ns)
ip link set veth1 netns $(ps -ef | grep '\[bash\]' | awk '{print $1}')

# assign ip  (container net ns)
ip addr add 10.0.1.2/24 dev veth1
ip link set veth1 up
 
# assign ip (host net ns)
ip addr add 10.0.1.1/24 dev veth0
ip link set veth0 up
```

到此，容器网络与主机网络已经打通，在容器中 `ping 10.0.1.1` ，能够确实地收到响应；并且在主机网络下通过 `tcpdump -i veth0` ，能抓到 **ICMP** 包

但现在在容器中的网络包仍然没法离开主机，访问公网资源。借助 **iptables**，我们来将容器与公网打通。

```sh
# force set ip route (container net ns)
ip route add default via 10.0.1.1 dev veth1

# set postrouting rule via iptables (host net ns)
iptables -t nat -A POSTROUTING -s 10.0.1.0/24 ! -d 10.0.1.0/24 -j SNAT --to-source [host ip]

# check filter FORWRD chain is ACCEPT (host net ns)
iptables-save -t filter | grep ':FORWARD'

# if not ACCPET (host net ns)
iptables -t filter -P FORWARD ACCEPT
```
