# Device File System文件系统管理
> 在早期的嵌入式系统中，需要存储的数据比较少，数据类型也比较单一，往往使用直接在存储设备中的指定地址写入数据的方法来存储数据。然而随着嵌入式设备功能的发展，需要存储的数据越来越多，也越来越复杂，这时仍使用旧方法来存储并管理数据就变得非常繁琐困难。因此我们需要新的数据管理方式来简化存储数据的组织形式，这种方式就是我们接下来要介绍的文件系统。

> 文件系统是一套实现了数据的存储、分级组织、访问和获取等操作的抽象数据类型 (Abstract data type)，是一种用于向用户提供底层数据访问的机制。文件系统通常存储的基本单位是文件，即数据是按照一个个文件的方式进行组织。当文件比较多时，将导致文件繁多，不易分类、重名的问题。而文件夹作为一个容纳多个文件的容器而存在。

# 一、DFS设备文件系统简介
## 1.1 DFS简介
DFS 是 RT-Thread 提供的虚拟文件系统组件，全称为 Device File System，即设备虚拟文件系统，文件系统的名称使用类似 UNIX 文件、文件夹的风格，目录结构如下图所示：
![DFS文件系统目录结构](https://img-blog.csdnimg.cn/20191020090025598.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
在 RT-Thread DFS 中，文件系统有统一的根目录，使用 / 来表示。而在根目录下的 f1.bin 文件则使用 /f1.bin 来表示，2018 目录下的 f1.bin 目录则使用 /data/2018/f1.bin 来表示。即目录的分割符号是 /，这与 UNIX/Linux 完全相同，与 Windows 则不相同（Windows 操作系统上使用 \ 来作为目录的分割符）。

## 1.2 DFS架构
DFS 的层次架构如下图所示，主要分为 POSIX 接口层、虚拟文件系统层和设备抽象层。
![DFS文件系统架构](https://img-blog.csdnimg.cn/20191020090220198.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)

 - **POSIX 接口层**：为应用程序提供统一的 POSIX 文件和目录操作接口：read、write、poll/select 等。

POSIX 表示可移植操作系统接口（Portable Operating System Interface of UNIX，缩写 POSIX），POSIX 标准定义了操作系统应该为应用程序提供的接口标准，是 IEEE 为要在各种 UNIX 操作系统上运行的软件而定义的一系列 API 标准的总称。

POSIX 标准意在期望获得源代码级别的软件可移植性。换句话说，为一个 POSIX 兼容的操作系统编写的程序，应该可以在任何其它 POSIX 操作系统（即使是来自另一个厂商）上编译执行。RT-Thread 支持 POSIX 标准接口，因此可以很方便的将 Linux/Unix 的程序移植到 RT-Thread 操作系统上。

在类 Unix 系统中，普通文件、设备文件、网络文件描述符是同一种文件描述符。而在 RT-Thread 操作系统中，使用 DFS 来实现这种统一性。有了这种文件描述符的统一性，我们就可以使用 poll/select 接口来对这几种描述符进行统一轮询，为实现程序功能带来方便。

使用 poll/select 接口可以阻塞地同时探测一组支持非阻塞的 I/O 设备是否有事件发生（如可读，可写，有高优先级的错误输出，出现错误等等），直至某一个设备触发了事件或者超过了指定的等待时间。这种机制可以帮助调用者寻找当前就绪的设备，降低编程的复杂度。

 - **虚拟文件系统层**：支持多种类型的文件系统，如 FatFS、RomFS、DevFS 等，并提供普通文件、设备文件、网络文件描述符的管理。

用户可以将具体的文件系统注册到 DFS 中，如 FatFS、RomFS、DevFS 等，下面介绍几种常用的文件系统类型：
| 文件系统类型 | 文件系统功能描述                                             |
| :----------- | :----------------------------------------------------------- |
| DevFS        | 设备文件系统，在 RT-Thread 操作系统中开启该功能后，可以将系统中的设备在 /dev 文件夹下虚拟成文件，使得设备可以按照文件的操作方式使用 read、write 等接口进行操作。 |
| elmfat FS    | 专为小型嵌入式设备开发的一个兼容微软 FAT 格式的文件系统，采用 ANSI C 编写，具有良好的硬件无关性以及可移植性，是 RT-Thread 中最常用的文件系统类型。 |
| Jffs2        | 一种日志闪存文件系统。主要用于 NOR 型闪存，基于 MTD 驱动层，特点是：可读写的、支持数据压缩的、基于哈希表的日志型文件系统，并提供了崩溃 / 掉电安全保护，提供写平衡支持等。 |
| NFS          | 网络文件系统（Network File System）是一项在不同机器、不同操作系统之间通过网络共享文件的技术。在操作系统的开发调试阶段，可以利用该技术在主机上建立基于 NFS 的根文件系统，挂载到嵌入式设备上，可以很方便地修改根文件系统的内容。 |
| RamFS        | 内存文件系统，它不能格式化，可以同时创建多个，在创建时可以指定其最大能使用的内存大小，优点是读写速度很快，但存在掉电丢失的风险。 |
| RomFS        | 一种简单的、紧凑的、只读的文件系统，不支持动态擦写保存，按顺序存放数据，因而支持应用程序以 XIP(execute In Place，片内运行) 方式运行，在系统运行时, 节省 RAM 空间。 |
| UFFS         | 超低功耗的闪存文件系统（Ultra-low-cost Flash File System）的简称。它是国人开发的、专为嵌入式设备等小内存环境中使用 Nand Flash 的开源文件系统。与嵌入式中常使用的 Yaffs 文件系统相比具有资源占用少、启动速度快、免费等优势。 |

 - **设备抽象层**：支持多种类型的存储设备，如 SD Card、SPI Flash、Nand Flash 等。

设备抽象层将物理设备如 SD Card、SPI Flash、Nand Flash，抽象成符合文件系统能够访问的设备，例如 FAT 文件系统要求存储设备必须是块设备类型。

不同文件系统类型是独立于存储设备驱动而实现的，因此把底层存储设备的驱动接口和文件系统对接起来之后，才可以正确地使用文件系统功能。

# 二、DFS文件系统管理
本文依然按照与[I / O设备模型框架](https://blog.csdn.net/m0_37621078/article/details/101158817)类似的形式介绍DFS设备文件系统，逐层介绍其描述数据结构、接口函数、调用过程等。在DFS虚拟文件系统层，我们选择RT-Thread最常用的elmfat作为示例，同时启用DevFS来管理设备。

## 2.1 DFS POSIX接口层
文件系统至少有三个要素构成：文件系统、文件、目录，其中目录可以看作是文件的一种类型，所以文件系统至少需要提供描述文件系统本身和文件的数据结构及相应的接口函数。

 - **文件系统控制块**

要想使用文件系统，需要先对其进行注册或挂载，要对文件系统操作，也需要对文件系统使用合理的数据结构进行描述，RT-Thread在DFS POSIX层对文件系统的描述如下：

```c
// rt-thread-4.0.1\components\dfs\include\dfs_fs.h

/* Mounted file system */
struct dfs_filesystem
{
    rt_device_t dev_id;     /* Attached device */

    char *path;             /* File system mount point */
    const struct dfs_filesystem_ops *ops; /* Operations for file system type */

    void *data;             /* Specific file system data */
};

/* File system operations */
struct dfs_filesystem_ops
{
    char *name;
    uint32_t flags;      /* flags for file system operations */

    /* operations for file */
    const struct dfs_file_ops *fops;

    /* mount and unmount file system */
    int (*mount)    (struct dfs_filesystem *fs, unsigned long rwflag, const void *data);
    int (*unmount)  (struct dfs_filesystem *fs);

    /* make a file system */
    int (*mkfs)     (rt_device_t devid);
    int (*statfs)   (struct dfs_filesystem *fs, struct statfs *buf);

    int (*unlink)   (struct dfs_filesystem *fs, const char *pathname);
    int (*stat)     (struct dfs_filesystem *fs, const char *filename, struct stat *buf);
    int (*rename)   (struct dfs_filesystem *fs, const char *oldpath, const char *newpath);
};


// rt-thread-4.0.1\components\dfs\src\dfs.c

/* Global variables */
const struct dfs_filesystem_ops *filesystem_operation_table[DFS_FILESYSTEM_TYPES_MAX];
struct dfs_filesystem filesystem_table[DFS_FILESYSTEMS_MAX];
```
dfs_filesystem结构体中成员dev_id是该文件系统要挂载的设备句柄，path为该文件系统要挂载的路径，ops则是该文件系统支持的接口函数集合，data则指向其私有数据。

在系统中可能不止挂载一种文件系统，所以多个文件系统的dfs_filesystem结构体与相应的dfs_filesystem_ops接口函数集合以全局数组的形式组织起来。

 - **文件控制块**

将需要的文件系统注册挂载成功后，用户使用文件系统主要是对文件的操作，在该文件系统中文件是如何描述的，支持哪些接口函数集合，也都需要相应的数据结构描述，RT-Thread在DFS POSIX层对文件的描述如下：

```c
// rt-thread-4.0.1\components\dfs\include\dfs_file.h

/* file descriptor */
#define DFS_FD_MAGIC     0xfdfd
struct dfs_fd
{
    uint16_t magic;              /* file descriptor magic number */
    uint16_t type;               /* Type (regular or socket) */

    char *path;                  /* Name (below mount point) */
    int ref_count;               /* Descriptor reference count */

    struct dfs_filesystem *fs;
    const struct dfs_file_ops *fops;

    uint32_t flags;              /* Descriptor flags */
    size_t   size;               /* Size in bytes */
    off_t    pos;                /* Current file position */

    void *data;                  /* Specific file system data */
};

struct dfs_file_ops
{
    int (*open)     (struct dfs_fd *fd);
    int (*close)    (struct dfs_fd *fd);
    int (*ioctl)    (struct dfs_fd *fd, int cmd, void *args);
    int (*read)     (struct dfs_fd *fd, void *buf, size_t count);
    int (*write)    (struct dfs_fd *fd, const void *buf, size_t count);
    int (*flush)    (struct dfs_fd *fd);
    int (*lseek)    (struct dfs_fd *fd, off_t offset);
    int (*getdents) (struct dfs_fd *fd, struct dirent *dirp, uint32_t count);

    int (*poll)     (struct dfs_fd *fd, struct rt_pollreq *req);
};


// rt-thread-4.0.1\components\dfs\include\dfs.h

struct dfs_fdtable
{
    uint32_t maxfd;
    struct dfs_fd **fds;
};

// rt-thread-4.0.1\components\dfs\src\dfs.c

static struct dfs_fdtable _fdtab;
```
dfs_fd文件描述结构体包含magic幻数、type文件类型、path文件路径包括文件名，ref_count文件引用次数、fs文件系统句柄、fops文件操作接口函数集合、flags文件打开标识、size文件占用字节数、pos当前文件位置、data文件私有数据指针等。

一个文件系统一般会管理多个文件，这些文件也以全局数组的形式组织管理，文件描述符表结构体dfs_fdtable有两个成员：最大文件数量和文件描述符表的首地址，这两个成员可以描述一个数组的元素个数与首地址。

 - **文件系统初始化与注册**

要想使用文件系统，需要先对其进行初始化，包括初始化文件系统的数据结构、将需要的文件系统挂载到指定设备的指定路径上。在DFS POSIC层的文件系统初始化过程如下：

```c
// rt-thread-4.0.1\components\dfs\src\dfs.c

/**
 * this function will initialize device file system.
 */
int dfs_init(void)
{
    static rt_bool_t init_ok = RT_FALSE;

    if (init_ok)
    {
        rt_kprintf("dfs already init.\n");
        return 0;
    }

    /* clear filesystem operations table */
    memset((void *)filesystem_operation_table, 0, sizeof(filesystem_operation_table));
    /* clear filesystem table */
    memset(filesystem_table, 0, sizeof(filesystem_table));
    /* clean fd table */
    memset(&_fdtab, 0, sizeof(_fdtab));

    /* create device filesystem lock */
    rt_mutex_init(&fslock, "fslock", RT_IPC_FLAG_FIFO);

#ifdef DFS_USING_WORKDIR
    /* set current working directory */
    memset(working_directory, 0, sizeof(working_directory));
    working_directory[0] = '/';
#endif

#ifdef RT_USING_DFS_DEVFS
    {
        extern int devfs_init(void);

        /* if enable devfs, initialize and mount it as soon as possible */
        devfs_init();

        dfs_mount(NULL, "/dev", "devfs", 0, 0);
    }
#endif

    init_ok = RT_TRUE;

    return 0;
}
INIT_PREV_EXPORT(dfs_init);
```
dfs_init函数完成了filesystem_operation_table、filesystem_table、_fdtab三个表的重置初始化操作，同时完成了devfs设备文件系统的初始化与挂载操作（devfs初始化与挂载操作需要下面devfs文件系统层的支持），当使能宏定义RT_USING_DFS_DEVFS时devfs不需要用户再次挂载即可使用。dfs_init函数使用RT-Thread自动初始化组件，在系统启动前自动调用完成DFS框架初始化。

参考I / O设备管理模型，一个对象初始化后需要将该对象的接口函数集合注册到RT-Thread对象管理层后才能通过统一的对象管理接口操作该对象。对于DFS设备文件系统来说，在对文件系统对象完成初始化后，需要将该文件系统的操作函数集合dfs_filesystem_ops注册到DFS设备文件系统层后，用户才能通过DFS设备文件系统接口函数来操作该文件系统。DFS注册过程如下：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_fs.c

/**
 * this function will register a file system instance to device file system.
 *  * @param ops the file system instance to be registered.
 *  * @return 0 on successful, -1 on failed.
 */
int dfs_register(const struct dfs_filesystem_ops *ops)
{
    int ret = RT_EOK;
    const struct dfs_filesystem_ops **empty = NULL;
    const struct dfs_filesystem_ops **iter;

    /* lock filesystem */
    dfs_lock();
    /* check if this filesystem was already registered */
    for (iter = &filesystem_operation_table[0];
            iter < &filesystem_operation_table[DFS_FILESYSTEM_TYPES_MAX]; iter ++)
    {
        /* find out an empty filesystem type entry */
        if (*iter == NULL)
            (empty == NULL) ? (empty = iter) : 0;
        else if (strcmp((*iter)->name, ops->name) == 0)
        {
            rt_set_errno(-EEXIST);
            ret = -1;
            break;
        }
    }

    /* save the filesystem's operations */
    if (empty == NULL)
    {
        rt_set_errno(-ENOSPC);
        LOG_E("There is no space to register this file system (%s).", ops->name);
        ret = -1;
    }
    else if (ret == RT_EOK)
    {
        *empty = ops;
    }

    dfs_unlock();
    return ret;
}
```
dfs_register函数主要是从filesystem_operation_table数组中找出一个空元素，然后将待注册的dfs_filesystem_ops作为参数赋值给filesystem_operation_table中找到的空元素，后面如果想操作某个文件系统，从filesystem_operation_table数组中根据名称查找到对应的文件系统接口函数集合，然后通过对应的接口函数指针调用该文件系统注册的操作函数即可。

 * **文件系统接口函数**

对文件系统的操作主要有创建、挂载、查询等操作。要想使用文件系统来管理文件，需要先将该文件系统挂载到设备某个位置（也即某个路径），要想将文件系统挂载到该设备某个路径下，需要先将该设备按照该文件系统的存储要求进行格式化，对该设备按文件系统需求进行格式化的过程称为在该设备上创建一个文件系统。文件系统在特定设备上创建，并挂载到特定路径后就可以正常使用该文件系统了，比如查询该文件系统的基本信息、使用该文件系统管理文件，也即在挂载路径下使用该文件系统提供的文件接口函数访问文件。

先看文件系统的创建过程：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_fs.c

/**
 * make a file system on the special device
 *
 * @param fs_name the file system name
 * @param device_name the special device name
 *
 * @return 0 on successful, otherwise failed.
 */
int dfs_mkfs(const char *fs_name, const char *device_name)
{
    int index;
    rt_device_t dev_id = NULL;

    /* check device name, and it should not be NULL */
    if (device_name != NULL)
        dev_id = rt_device_find(device_name);

    if (dev_id == NULL)
    {
        rt_set_errno(-ENODEV);
        LOG_E("Device (%s) was not found", device_name);
        return -1;
    }

    /* lock file system */
    dfs_lock();
    /* find the file system operations */
    for (index = 0; index < DFS_FILESYSTEM_TYPES_MAX; index ++)
    {
        if (filesystem_operation_table[index] != NULL &&
            strcmp(filesystem_operation_table[index]->name, fs_name) == 0)
            break;
    }
    dfs_unlock();

    if (index < DFS_FILESYSTEM_TYPES_MAX)
    {
        /* find file system operation */
        const struct dfs_filesystem_ops *ops = filesystem_operation_table[index];
        if (ops->mkfs == NULL)
        {
            LOG_E("The file system (%s) mkfs function was not implement", fs_name);
            rt_set_errno(-ENOSYS);
            return -1;
        }

        return ops->mkfs(dev_id);
    }

    LOG_E("File system (%s) was not found.", fs_name);

    return -1;
}
```
dfs_mkfs函数最终调用的是下面DFS虚拟文件系统层注册的ops->mkfs函数，该函数最终由要使用的文件系统实现。

下面看文件系统的挂载过程：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_fs.c

/**
 * this function will mount a file system on a specified path.
 *
 * @param device_name the name of device which includes a file system.
 * @param path the path to mount a file system
 * @param filesystemtype the file system type
 * @param rwflag the read/write etc. flag.
 * @param data the private data(parameter) for this file system.
 *
 * @return 0 on successful or -1 on failed.
 */
int dfs_mount(const char   *device_name,
              const char   *path,
              const char   *filesystemtype,
              unsigned long rwflag,
              const void   *data)
{
    const struct dfs_filesystem_ops **ops;
    struct dfs_filesystem *iter;
    struct dfs_filesystem *fs = NULL;
    char *fullpath = NULL;
    rt_device_t dev_id;

    /* open specific device */
    if (device_name == NULL)
    {
        /* which is a non-device filesystem mount */
        dev_id = NULL;
    }
    else if ((dev_id = rt_device_find(device_name)) == NULL)
    {
        /* no this device */
        rt_set_errno(-ENODEV);
        return -1;
    }

    /* find out the specific filesystem */
    dfs_lock();

    for (ops = &filesystem_operation_table[0];
            ops < &filesystem_operation_table[DFS_FILESYSTEM_TYPES_MAX]; ops++)
        if ((*ops != NULL) && (strcmp((*ops)->name, filesystemtype) == 0))
            break;

    dfs_unlock();

    if (ops == &filesystem_operation_table[DFS_FILESYSTEM_TYPES_MAX])
    {
        /* can't find filesystem */
        rt_set_errno(-ENODEV);
        return -1;
    }

    /* check if there is mount implementation */
    if ((*ops == NULL) || ((*ops)->mount == NULL))
    {
        rt_set_errno(-ENOSYS);
        return -1;
    }

    /* make full path for special file */
    fullpath = dfs_normalize_path(NULL, path);
    if (fullpath == NULL) /* not an abstract path */
    {
        rt_set_errno(-ENOTDIR);
        return -1;
    }

    /* Check if the path exists or not, raw APIs call, fixme */
    if ((strcmp(fullpath, "/") != 0) && (strcmp(fullpath, "/dev") != 0))
    {
        struct dfs_fd fd;

        if (dfs_file_open(&fd, fullpath, O_RDONLY | O_DIRECTORY) < 0)
        {
            rt_free(fullpath);
            rt_set_errno(-ENOTDIR);

            return -1;
        }
        dfs_file_close(&fd);
    }

    /* check whether the file system mounted or not  in the filesystem table
     * if it is unmounted yet, find out an empty entry */
    dfs_lock();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[DFS_FILESYSTEMS_MAX]; iter++)
    {
        /* check if it is an empty filesystem table entry? if it is, save fs */
        if (iter->ops == NULL)
            (fs == NULL) ? (fs = iter) : 0;
        /* check if the PATH is mounted */
        else if (strcmp(iter->path, path) == 0)
        {
            rt_set_errno(-EINVAL);
            goto err1;
        }
    }

    if ((fs == NULL) && (iter == &filesystem_table[DFS_FILESYSTEMS_MAX]))
    {
        rt_set_errno(-ENOSPC);
        LOG_E("There is no space to mount this file system (%s).", filesystemtype);
        goto err1;
    }

    /* register file system */
    fs->path   = fullpath;
    fs->ops    = *ops;
    fs->dev_id = dev_id;
    /* release filesystem_table lock */
    dfs_unlock();

    /* open device, but do not check the status of device */
    if (dev_id != NULL)
    {
        if (rt_device_open(fs->dev_id,
                           RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        {
            /* The underlaying device has error, clear the entry. */
            dfs_lock();
            memset(fs, 0, sizeof(struct dfs_filesystem));

            goto err1;
        }
    }

    /* call mount of this filesystem */
    if ((*ops)->mount(fs, rwflag, data) < 0)
    {
        /* close device */
        if (dev_id != NULL)
            rt_device_close(fs->dev_id);

        /* mount failed */
        dfs_lock();
        /* clear filesystem table entry */
        memset(fs, 0, sizeof(struct dfs_filesystem));

        goto err1;
    }

    return 0;

err1:
    dfs_unlock();
    rt_free(fullpath);

    return -1;
}
```
dfs_mount函数最终也是通过调用下层注册的(*ops)->mount函数实现，该函数最终由要使用的文件系统实现。

文件系统操作还有其他的一些接口函数，下面只给出函数声明：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_fs.c

/**
 * this function will unmount a file system on specified path.
 *
 * @param specialfile the specified path which mounted a file system.
 *
 * @return 0 on successful or -1 on failed.
 */
int dfs_unmount(const char *specialfile);

/**
 * this function will return the information about a mounted file system.
 *
 * @param path the path which mounted file system.
 * @param buffer the buffer to save the returned information.
 *
 * @return 0 on successful, others on failed.
 */
int dfs_statfs(const char *path, struct statfs *buffer);


// rt-thread-4.0.1\components\dfs\include\dfs.h
struct statfs
{
    size_t f_bsize;   /* block size */
    size_t f_blocks;  /* total data blocks in file system */
    size_t f_bfree;   /* free blocks in file system */
};
```

DFS POSIX接口主要是针对文件和目录的操作，对文件系统的操作比较少，也只提供了对文件系统的信息查询接口，实际调用的还是上面的dfs_statfs函数，DFS POSIX文件系统API如下：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_posix.c

/**
 * this function is a POSIX compliant version, which will return the
 * information about a mounted file system.
 *
 * @param path the path which mounted file system.
 * @param buf the buffer to save the returned information.
 *
 * @return 0 on successful, others on failed.
 */
int statfs(const char *path, struct statfs *buf)
{
    int result;

    result = dfs_statfs(path, buf);
    if (result < 0)
    {
        rt_set_errno(result);

        return -1;
    }

    return result;
}
RTM_EXPORT(statfs);
```

 * **文件访问接口函数**

文件系统注册接口函数集合时，连同文件接口函数集合也一起注册了，回顾下前面dfs_filesystem_ops结构体中有一个成员dfs_file_ops *指向该文件系统中文件操作接口函数集合的地址。当文件系统挂载成功后，就可以直接使用该文件系统提供的文件操作接口函数对文件进行访问了。

对文件的访问最终还是通过调用该文件系统向上层注册的文件访问函数实现的，下面以文件打开函数为例，看下这个过程：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_file.c

/**
 * this function will open a file which specified by path with specified flags.
 *
 * @param fd the file descriptor pointer to return the corresponding result.
 * @param path the specified file path.
 * @param flags the flags for open operator.
 *
 * @return 0 on successful, -1 on failed.
 */
int dfs_file_open(struct dfs_fd *fd, const char *path, int flags)
{
    struct dfs_filesystem *fs;
    char *fullpath;
    int result;

    /* parameter check */
    if (fd == NULL)
        return -EINVAL;

    /* make sure we have an absolute path */
    fullpath = dfs_normalize_path(NULL, path);
    if (fullpath == NULL)
    {
        return -ENOMEM;
    }

    LOG_D("open file:%s", fullpath);

    /* find filesystem */
    fs = dfs_filesystem_lookup(fullpath);
    if (fs == NULL)
    {
        rt_free(fullpath); /* release path */

        return -ENOENT;
    }

    LOG_D("open in filesystem:%s", fs->ops->name);
    fd->fs    = fs;             /* set file system */
    fd->fops  = fs->ops->fops;  /* set file ops */

    /* initialize the fd item */
    fd->type  = FT_REGULAR;
    fd->flags = flags;
    fd->size  = 0;
    fd->pos   = 0;
    fd->data  = fs;

    if (!(fs->ops->flags & DFS_FS_FLAG_FULLPATH))
    {
        if (dfs_subdir(fs->path, fullpath) == NULL)
            fd->path = rt_strdup("/");
        else
            fd->path = rt_strdup(dfs_subdir(fs->path, fullpath));
        rt_free(fullpath);
        LOG_D("Actual file path: %s", fd->path);
    }
    else
    {
        fd->path = fullpath;
    }

    /* specific file system open routine */
    if (fd->fops->open == NULL)
    {
        /* clear fd */
        rt_free(fd->path);
        fd->path = NULL;

        return -ENOSYS;
    }

    if ((result = fd->fops->open(fd)) < 0)
    {
        /* clear fd */
        rt_free(fd->path);
        fd->path = NULL;

        LOG_D("%s open failed", fullpath);

        return result;
    }

    fd->flags |= DFS_F_OPEN;
    if (flags & O_DIRECTORY)
    {
        fd->type = FT_DIRECTORY;
        fd->flags |= DFS_F_DIRECTORY;
    }

    LOG_D("open successful");
    return 0;
}
```
在dfs_file_open中调用fd->fops->open（见：result = fd->fops->open(fd)）函数，足以说明DFS层的文件访问接口函数最终是通过调用所挂载文件系统内部提供的文件访问函数实现的。

从上面函数传入的参数可以看出，跟我们在Linux上使用的文件访问接口函数并不一致，我们更习惯把文件名作为参数传入，而不是把文件描述符作为参数传入。文件描述符信息较多，我们配置起来并不方便，为了兼容LInux的文件访问接口，DFS又提供了POSIX风格的接口函数，下面仍以文件打开函数为例，看POSIX风格的文件访问接口函数的实现过程：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_posix.c

/**
 * this function is a POSIX compliant version, which will open a file and
 * return a file descriptor according specified flags.
 *
 * @param file the path name of file.
 * @param flags the file open flags.
 *
 * @return the non-negative integer on successful open, others for failed.
 */
int open(const char *file, int flags, ...)
{
    int fd, result;
    struct dfs_fd *d;

    /* allocate a fd */
    fd = fd_new();
    if (fd < 0)
    {
        rt_set_errno(-ENOMEM);

        return -1;
    }
    d = fd_get(fd);

    result = dfs_file_open(d, file, flags);
    if (result < 0)
    {
        /* release the ref-count of fd */
        fd_put(d);
        fd_put(d);

        rt_set_errno(result);

        return -1;
    }

    /* release the ref-count of fd */
    fd_put(d);

    return fd;
}
RTM_EXPORT(open);
```
DFS POSIX风格的接口函数跟Linux一致，我们使用也更加方便，open函数实际上是对dfs_file_open函数的再封装，并且隐藏了文件描述符的配置管理过程，使用起来比较方便。

上面open函数中调用的fd_new / fd_get / fd_put是文件描述符管理函数，提供对文件描述符结构体的配置管理功能，这几个函数在.\components\dfs\src\dfs.c中实现，这里就不再展示其代码实现了。

DFS POSIX提供的文件访问接口还有很多，下面只展示其接口函数声明：

```c
// rt-thread-4.0.1\components\dfs\src\dfs_posix.c

/**
 * this function is a POSIX compliant version, which will open a file and
 * return a file descriptor according specified flags.
 *
 * @param file the path name of file.
 * @param flags the file open flags.
 *
 * @return the non-negative integer on successful open, others for failed.
 */
int open(const char *file, int flags, ...);

/**
 * this function is a POSIX compliant version, which will close the open
 * file descriptor.
 *
 * @param fd the file descriptor.
 *
 * @return 0 on successful, -1 on failed.
 */
int close(int fd);

/**
 * this function is a POSIX compliant version, which will read specified data
 * buffer length for an open file descriptor.
 *
 * @param fd the file descriptor.
 * @param buf the buffer to save the read data.
 * @param len the maximal length of data buffer
 *
 * @return the actual read data buffer length. If the returned value is 0, it
 * may be reach the end of file, please check errno.
 */
int read(int fd, void *buf, size_t len);

/**
 * this function is a POSIX compliant version, which will write specified data
 * buffer length for an open file descriptor.
 *
 * @param fd the file descriptor
 * @param buf the data buffer to be written.
 * @param len the data buffer length.
 *
 * @return the actual written data buffer length.
 */
int write(int fd, const void *buf, size_t len);

/**
 * this function is a POSIX compliant version, which will seek the offset for
 * an open file descriptor.
 *
 * @param fd the file descriptor.
 * @param offset the offset to be seeked.
 * @param whence the directory of seek.
 *
 * @return the current read/write position in the file, or -1 on failed.
 */
off_t lseek(int fd, off_t offset, int whence);

/**
 * this function is a POSIX compliant version, which will rename old file name
 * to new file name.
 *
 * @param old the old file name.
 * @param new the new file name.
 *
 * @return 0 on successful, -1 on failed.
 *
 * note: the old and new file name must be belong to a same file system.
 */
int rename(const char *old, const char *new);

/**
 * this function is a POSIX compliant version, which will unlink (remove) a
 * specified path file from file system.
 *
 * @param pathname the specified path name to be unlinked.
 *
 * @return 0 on successful, -1 on failed.
 */
int unlink(const char *pathname);

/**
 * this function is a POSIX compliant version, which will get file information.
 *
 * @param file the file name
 * @param buf the data buffer to save stat description.
 *
 * @return 0 on successful, -1 on failed.
 */
int stat(const char *file, struct stat *buf);

/**
 * this function is a POSIX compliant version, which will get file status.
 *
 * @param fildes the file description
 * @param buf the data buffer to save stat description.
 *
 * @return 0 on successful, -1 on failed.
 */
int fstat(int fildes, struct stat *buf);

/**
 * this function is a POSIX compliant version, which shall request that all data
 * for the open file descriptor named by fildes is to be transferred to the storage
 * device associated with the file described by fildes.
 *
 * @param fildes the file description
 *
 * @return 0 on successful completion. Otherwise, -1 shall be returned and errno
 * set to indicate the error.
 */
int fsync(int fildes);

/**
 * this function is a POSIX compliant version, which shall perform a variety of
 * control functions on devices.
 *
 * @param fildes the file description
 * @param cmd the specified command
 * @param data represents the additional information that is needed by this
 * specific device to perform the requested function.
 *
 * @return 0 on successful completion. Otherwise, -1 shall be returned and errno
 * set to indicate the error.
 */
int fcntl(int fildes, int cmd, ...);

/**
 * this function is a POSIX compliant version, which shall perform a variety of
 * control functions on devices.
 *
 * @param fildes the file description
 * @param cmd the specified command
 * @param data represents the additional information that is needed by this
 * specific device to perform the requested function.
 *
 * @return 0 on successful completion. Otherwise, -1 shall be returned and errno
 * set to indicate the error.
 */
int ioctl(int fildes, int cmd, ...);


// rt-thread-4.0.1\components\dfs\src\select.c

/**
 * This function can monitor the I/O device for events.
 *
 * @param nfds the maximum value of all file descriptors plus 1
 * @param readfds Set of read event file descriptors that need to be monitored
 * @param writefds Set of write event file descriptors that need to be monitored
 * @param exceptfds Set of exception event file descriptors that need to be monitored
 *
 * @return >0 A read/write event or error occurred in the monitored file collection. 
 * 		   =0 Waiting for timeout, no readable or writable or erroneous files
 * 		   <0 Error
 */
 int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```
select函数比较特殊复杂些，可以阻塞的同时探测一组支持非阻塞的I / O设备是否有事件发生（比如可读、可写、出现异常或错误等），直至某一个设备触发了事件或者超过了指定的等待时间。该函数的实现原理在[《Socket API编程：基于Select的并发服务器》](https://blog.csdn.net/m0_37621078/article/details/99053518)中有详细介绍，在lwip中把套接字作文文件描述符对象进行管理的。

目录也是一种特殊类型的文件，目录跟文件还是有些不同，所以DFS POSIC针对目录操作也提供了一组POSIX接口函数集合（实际调用的依然是dfs_file_ops函数集合），目录的数据描述及接口函数声明如下：

```c
// rt-thread-4.0.1\components\dfs\include\dfs_posix.h
typedef struct
{
    int fd;     /* directory file */
    char buf[512];
    int num;
    int cur;
} DIR;


// rt-thread-4.0.1\components\dfs\src\dfs_posix.c

/**
 * this function is a POSIX compliant version, which will make a directory
 *
 * @param path the directory path to be made.
 * @param mode
 *
 * @return 0 on successful, others on failed.
 */
int mkdir(const char *path, mode_t mode);

/**
 * this function is a POSIX compliant version, which will remove a directory.
 *
 * @param pathname the path name to be removed.
 *
 * @return 0 on successful, others on failed.
 */
int rmdir(const char *pathname);

/**
 * this function is a POSIX compliant version, which will open a directory.
 *
 * @param name the path name to be open.
 *
 * @return the DIR pointer of directory, NULL on open directory failed.
 */
DIR *opendir(const char *name);

/**
 * this function is a POSIX compliant version, which will return a pointer
 * to a dirent structure representing the next directory entry in the
 * directory stream.
 *
 * @param d the directory stream pointer.
 *
 * @return the next directory entry, NULL on the end of directory or failed.
 */
struct dirent *readdir(DIR *d);

/**
 * this function is a POSIX compliant version, which will return current
 * location in directory stream.
 *
 * @param d the directory stream pointer.
 *
 * @return the current location in directory stream.
 */
long telldir(DIR *d);

/**
 * this function is a POSIX compliant version, which will set position of
 * next directory structure in the directory stream.
 *
 * @param d the directory stream.
 * @param offset the offset in directory stream.
 */
void seekdir(DIR *d, off_t offset);

/**
 * this function is a POSIX compliant version, which will reset directory
 * stream.
 *
 * @param d the directory stream.
 */
void rewinddir(DIR *d);

/**
 * this function is a POSIX compliant version, which will close a directory
 * stream.
 *
 * @param d the directory stream.
 *
 * @return 0 on successful, -1 on failed.
 */
int closedir(DIR *d);

/**
 * this function is a POSIX compliant version, which will change working
 * directory.
 *
 * @param path the path name to be changed to.
 *
 * @return 0 on successful, -1 on failed.
 */
int chdir(const char *path);

/**
 * this function is a POSIX compliant version, which will return current
 * working directory.
 *
 * @param buf the returned current directory.
 * @param size the buffer size.
 *
 * @return the returned current directory.
 */
char *getcwd(char *buf, size_t size);
```

上面这些DFS POSIX接口函数是在挂载文件系统后，用户使用文件系统访问文件或目录实际调用的接口函数，使用比较频繁。

## 2.2 DFS虚拟文件系统层
DFS虚拟文件系统层支持多种类型的文件系统，这里直介绍跟I / O设备管理模型配合的DevFS文件系统和最常用的elmfat文件系统。

### 2.2.1 devfs设备文件系统
首先看devfs文件系统的初始化过程：

```c
// rt-thread-4.0.1\components\dfs\filesystems\devfs\devfs.c

int devfs_init(void)
{
    /* register rom file system */
    dfs_register(&_device_fs);

    return 0;
}

static const struct dfs_filesystem_ops _device_fs =
{
    "devfs",
    DFS_FS_FLAG_DEFAULT,
    &_device_fops,

    dfs_device_fs_mount,
    RT_NULL,
    RT_NULL,
    RT_NULL,

    RT_NULL,
    dfs_device_fs_stat,
    RT_NULL,
};

static const struct dfs_file_ops _device_fops =
{
    dfs_device_fs_open,
    dfs_device_fs_close,
    dfs_device_fs_ioctl,
    dfs_device_fs_read,
    dfs_device_fs_write,
    RT_NULL,                    /* flush */
    RT_NULL,                    /* lseek */
    dfs_device_fs_getdents,
    dfs_device_fs_poll,
};
```

前面介绍DFS POSIX时，devfs_init函数被dfs_init调用，不过需要定义条件宏RT_USING_DFS_DEVFS，也即在menuconfig中配置devfs启用后，devfs_init便会被自动调用，同时也会自动挂载devfs设备文件系统。

devfs主要用来管理I / O设备，向DFS POSIX注册的文件操作集合_device_fops最终还是通过调用 I / O设备管理层接口函数实现的，相当于在I / O设备管理框架基础上再封装一层DFS POSIX设备文件系统层，相当于可以直接用DFS POSIX访问文件的接口函数来访问注册到I / O设备管理框架的设备。下面以一个函数示例说明这种调用关系：

```c
// rt-thread-4.0.1\components\dfs\filesystems\devfs\devfs.c

struct device_dirent
{
    rt_device_t *devices;
    rt_uint16_t read_index;
    rt_uint16_t device_count;
};

int dfs_device_fs_open(struct dfs_fd *file)
{
    rt_err_t result;
    rt_device_t device;

    /* open root directory */
    if ((file->path[0] == '/') && (file->path[1] == '\0') &&
        (file->flags & O_DIRECTORY))
    {
        struct rt_object *object;
        struct rt_list_node *node;
        struct rt_object_information *information;
        struct device_dirent *root_dirent;
        rt_uint32_t count = 0;

        /* lock scheduler */
        rt_enter_critical();

        /* traverse device object */
        information = rt_object_get_information(RT_Object_Class_Device);
        RT_ASSERT(information != RT_NULL);
        for (node = information->object_list.next; node != &(information->object_list); node = node->next)
        {
            count ++;
        }

        root_dirent = (struct device_dirent *)rt_malloc(sizeof(struct device_dirent) +
                      count * sizeof(rt_device_t));
        if (root_dirent != RT_NULL)
        {
            root_dirent->devices = (rt_device_t *)(root_dirent + 1);
            root_dirent->read_index = 0;
            root_dirent->device_count = count;
            count = 0;
            /* get all device node */
            for (node = information->object_list.next; node != &(information->object_list); node = node->next)
            {
                object = rt_list_entry(node, struct rt_object, list);
                root_dirent->devices[count] = (rt_device_t)object;
                count ++;
            }
        }
        rt_exit_critical();

        /* set data */
        file->data = root_dirent;

        return RT_EOK;
    }

    device = rt_device_find(&file->path[1]);
    if (device == RT_NULL)
        return -ENODEV;

#ifdef RT_USING_POSIX
    if (device->fops)
    {
        /* use device fops */
        file->fops = device->fops;
        file->data = (void *)device;

        /* use fops */
        if (file->fops->open)
        {
            result = file->fops->open(file);
            if (result == RT_EOK || result == -RT_ENOSYS)
            {
                return 0;
            }
        }
    }
    else
#endif
    {
        result = rt_device_open(device, RT_DEVICE_OFLAG_RDWR);
        if (result == RT_EOK || result == -RT_ENOSYS)
        {
            file->data = device;
            return RT_EOK;
        }
    }

    file->data = RT_NULL;
    /* open device failed. */
    return -EIO;
}

int dfs_device_fs_read(struct dfs_fd *file, void *buf, size_t count)
{
    int result;
    rt_device_t dev_id;

    RT_ASSERT(file != RT_NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->data;
    RT_ASSERT(dev_id != RT_NULL);

    /* read device data */
    result = rt_device_read(dev_id, file->pos, buf, count);
    file->pos += result;

    return result;
}
```
为了便于通过路径文件名的方式访问设备，增加了设备目录描述结构体device_dirent，结构体device_dirent首成员指向设备对象rt_device的句柄，可以通过结构体device_dirent方便的获取已注册设备句柄，并通过设备句柄访问该设备。

结构体device_dirent首成员devices实际上是一个指针数组的首地址，数组元素是设备句柄（指针），数组索引和总元素个数由device_dirent的另外两个成员定义。

结构体device_dirent对象的首地址又被赋值给文件描述符的私有数据成员dfs_fd.data，这样就可以通过文件描述符dfs_fd方便的访问到相应的设备rt_device，实现DFS POSIX接口函数参数到 I / O 设备管理接口函数参数的转换。如果查找设备，文件路径名就包含了设备名，通过file->path[1]便可获得设备名，作为rt_device_find的传入参数。

上面的函数dfs_device_fs_open实际调用的是device->fops->open或rt_device_open，其中后者rt_device_open是我们介绍 I / O设备模型框架时常用的接口形式，前者在介绍设备对象rt_device时介绍过一个成员dfs_file_ops只有在定义条件宏RT_USING_POSIX时有效，成员dfs_file_ops恰是为了支持DFS设备文件系统而定义的。

### 2.2.2 elmfat虚拟文件系统
elmfat文件系统比devfs更复杂，我们先看下elmfat文件结构及文件依赖关系图：
![elmfat文件依赖关系图](https://img-blog.csdnimg.cn/20191021225016610.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
RT-Thread为了适配DFS框架，在此基础上新增了dfs_elm.c与dfs_elm.h两个文件，主要对ff.h内elmfat文件系统提供的接口按照DFS中dfs_filesystem_ops与dfs_file_ops的格式进行再封装，同时把封装好的符合DFS要求的文件系统接口函数集合dfs_elm注册到DFS框架中去。

elmfat文件系统初始化和注册过程如下：

```c
// rt-thread-4.0.1\components\dfs\filesystems\elmfat\dfs_elm.c

int elm_init(void)
{
    /* register fatfs file system */
    dfs_register(&dfs_elm);

    return 0;
}
INIT_COMPONENT_EXPORT(elm_init);

static const struct dfs_filesystem_ops dfs_elm =
{
    "elm",
    DFS_FS_FLAG_DEFAULT,
    &dfs_elm_fops,

    dfs_elm_mount,
    dfs_elm_unmount,
    dfs_elm_mkfs,
    dfs_elm_statfs,

    dfs_elm_unlink,
    dfs_elm_stat,
    dfs_elm_rename,
};

static const struct dfs_file_ops dfs_elm_fops =
{
    dfs_elm_open,
    dfs_elm_close,
    dfs_elm_ioctl,
    dfs_elm_read,
    dfs_elm_write,
    dfs_elm_flush,
    dfs_elm_lseek,
    dfs_elm_getdents,
    RT_NULL, /* poll interface */
};
```
elmfat初始化函数被自动初始化组件调用，不需要用户再手动调用了。elmfat文件系统向上层DFS注册的接口函数集合就不展开讲了（如果展开介绍还需要再介绍ff.h与ff.c中elmfat文件系统的具体实现过程）。

要移植elmfat文件系统，还需要了解其对底层 I / O设备的操作，从elmfat文件结构可知，该部分接口在diskio.h中声明，要移植elmfat文件系统，需要我们把diskio.h中声明的函数实现出来。RT-Thread同样帮我们在dfs_elm.c中实现了，移植函数的实现如下：

```c
// rt-thread-4.0.1\components\dfs\filesystems\elmfat\dfs_elm.c

/* Read Sector(s) */
DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, UINT count)
{
    rt_size_t result;
    rt_device_t device = disk[drv];

    result = rt_device_read(device, sector, buff, count);
    if (result == count)
    {
        return RES_OK;
    }

    return RES_ERROR;
}

/* Write Sector(s) */
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, UINT count)
{
    rt_size_t result;
    rt_device_t device = disk[drv];

    result = rt_device_write(device, sector, buff, count);
    if (result == count)
    {
        return RES_OK;
    }

    return RES_ERROR;
}

/* Miscellaneous Functions */
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    rt_device_t device = disk[drv];

    if (device == RT_NULL)
        return RES_ERROR;

    if (ctrl == GET_SECTOR_COUNT)
    {
        struct rt_device_blk_geometry geometry;

        rt_memset(&geometry, 0, sizeof(geometry));
        rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);

        *(DWORD *)buff = geometry.sector_count;
        if (geometry.sector_count == 0)
            return RES_ERROR;
    }
    else if (ctrl == GET_SECTOR_SIZE)
    {
        struct rt_device_blk_geometry geometry;

        rt_memset(&geometry, 0, sizeof(geometry));
        rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);

        *(WORD *)buff = (WORD)(geometry.bytes_per_sector);
    }
    else if (ctrl == GET_BLOCK_SIZE) /* Get erase block size in unit of sectors (DWORD) */
    {
        struct rt_device_blk_geometry geometry;

        rt_memset(&geometry, 0, sizeof(geometry));
        rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);

        *(DWORD *)buff = geometry.block_size / geometry.bytes_per_sector;
    }
    else if (ctrl == CTRL_SYNC)
    {
        rt_device_control(device, RT_DEVICE_CTRL_BLK_SYNC, RT_NULL);
    }
    else if (ctrl == CTRL_TRIM)
    {
        rt_device_control(device, RT_DEVICE_CTRL_BLK_ERASE, buff);
    }

    return RES_OK;
}
```
从上面的代码可以看出，elmfat文件系统对磁盘设备的操作函数最终是通过调用 I / O设备管理接口函数实现的。

elmfat只能挂载到块设备上，比如SPI / QSPI Flash或SD Card等，前篇文章才介绍过[SPI设备对象管理与SFUD管理](https://blog.csdn.net/m0_37621078/article/details/102559086)，这里比较方便的办法是通过SFUD框架管理SPI / QSPI Flash，SFUD框架也会向 I / O设备管理层注册统一的接口函数集合，下面我们在前一篇[通过SFUD访问W25Q128示例](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_device_sample)的基础上使用DFS文件系统。

## 2.3 DFS设备抽象层
DFS设备抽象层主要用于驱动具体的 I / O块设备，前一篇介绍的SFUD管理SPI / QSPI Flash的框架就属于DFS设备抽象层，这里就不再赘述了。

SFUD框架一般按物理设备的个数为单位驱动的，也即一个SPI Flash（比如W25Q128）为一个块设备，如果我们想对一个Flash分为多个逻辑设备或者多个分区，每个分区的作用不同或者挂载不同的文件系统，也即只想在一个Flash的一个分区上挂载某文件系统（比如elmfat），可以借助FAL（Flash Abstraction Layer)）Flash抽象层来实现该需求。

FAL可对多个物理Flash统一管理，也可能将某一物理Flash划分为多个逻辑分区，可以在每个分区上创建块设备，用于上层文件系统的挂载，FAL框架图如下：
![FAL框架图](https://img-blog.csdnimg.cn/20191021234619558.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
FAL Flash抽象层并非必要，这里限于篇幅留待后面再介绍，下面先基于SFUD框架展示DFS devfs与elmfat文件系统的使用示例。

# 三、DFS文件系统示例
这里在示例工程[通过SFUD访问W25Q128示例](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_device_sample)的基础上新增，为了突出DFS文件系统组件，将stm32l475_device_sample复制一份并改名stm32l475_dfs_sample，DFS示例工程在stm32l475_dfs_sample的基础上新增。

## 3.1 devfs文件系统示例
切换到工程目录stm32l475_dfs_sample，打开env输入menuconfig，启用Device Virtual  file system与devfs的配置界面如下：
![启用devfs配置界面](https://img-blog.csdnimg.cn/20191022190435955.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
保存配置并退出，在projects\stm32l475_dfs_sample\applications目录下新建文件dfs_sample.c（将原先的pin_sample.c与spi_sample.c移到新建文件夹\applications\old_sample中不参与编译），打开文件dfs_sample.c编写devfs示例代码（我使用的[VS Code代码编译器](https://blog.csdn.net/m0_37621078/article/details/88320010)）。

本示例想同时包含目录访问与文件访问，devfs主要用于访问挂载到“/dev”目录下的设备，所以本示例先读取/dev目录下所有的设备，再通过DFS POSIX接口访问PIN设备中的蜂鸣器，实现配置蜂鸣器模式、读写蜂鸣器状态的功能。按照实现目标编写的代码如下：

```c
// projects\stm32l475_dfs_sample\applications\dfs_sample.c

#include "rtdevice.h"
#include "rtthread.h"
#include "board.h"
#include "dfs_posix.h"


#define BEEP_PIN    GET_PIN(B, 2)

static void devfs_sample(void)
{
    int fd;
    DIR *dirp;
    struct dirent *d;
    struct rt_device_pin_mode beepmode;
    struct rt_device_pin_status beepstatus, tempstatus;

    dirp = opendir("/dev");
    if(dirp == RT_NULL){
        rt_kprintf("open directory error!\n");
    }else{
        while ((d = readdir(dirp)) != RT_NULL){
            rt_kprintf("found device: %s\n", d->d_name);
        }
        closedir(dirp);
    }

    beepmode.pin = beepstatus.pin = tempstatus.pin = BEEP_PIN;
    beepmode.mode = PIN_MODE_OUTPUT;
    beepstatus.status = PIN_HIGH;

    rt_kprintf("Control beep via devfs.\n");
    
	/* 打开设备文件的标识RT_DEVICE_OFLAG_OPEN */
    fd = open("/dev/pin", RT_DEVICE_OFLAG_OPEN);
    if(fd >= 0){
        if(ioctl(fd, 0, &beepmode) == 0)
            rt_kprintf("Beep pin mode config finish.\n");

        if(write(fd, &beepstatus, sizeof(beepstatus)) == sizeof(beepstatus))
            rt_kprintf("Beep turn on.\n");

        if(read(fd, &tempstatus, sizeof(tempstatus)) == sizeof(tempstatus))
            rt_kprintf("Beep pin status: %d.\n", tempstatus.status);

        close(fd);
    }

    rt_thread_mdelay(3000);

    beepstatus.status = PIN_LOW;

    fd = open("/dev/pin", RT_DEVICE_OFLAG_OPEN);
    if(fd >= 0){
        if(write(fd, &beepstatus, sizeof(beepstatus)) == sizeof(beepstatus))
            rt_kprintf("Beep turn off.\n");

        if(read(fd, &tempstatus, sizeof(tempstatus)) == sizeof(tempstatus))
            rt_kprintf("Beep pin status: %d.\n", tempstatus.status);

        close(fd);
    }
}
MSH_CMD_EXPORT(devfs_sample, devfs sample);
```
在env环境中执行scons --target=mdk5生成MDK5工程代码，打开project.uvprojx编译报错，将编译器配置为ARM Compiler V5，重新编译无报错。将程序烧录到STM32L475潘多拉开发板上，运行结果如下：
![devfs示例运行结果](https://img-blog.csdnimg.cn/20191022193254272.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
从串口工具putty通过finsh组件交互结果看，运行devfs_sample示例程序运行正常，通过读取目录/dev下的设备文件和list_device命令获取的设备列表一致，通过DFS POSIX接口也可以正常控制蜂鸣器PIN设备。

DFS除了提供POSIX接口函数外，还提供了FINSH部分命令，便于用户通过finsh访问设备文件和目录。

本示例工程源码下载地址：[https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_dfs_sample](https://github.com/StreamAI/RT-Thread_Projects/tree/master/projects/stm32l475_dfs_sample)

## 3.2 elmfat文件系统示例
该工作目录env环境输入menuconfig命令，新增elmfat文件系统配置如下：
![elmfat启用配置](https://img-blog.csdnimg.cn/20191022193736211.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
需要注意的是，elmfat文件系统默认的最大扇区大小maximum sector size是512，在前篇博客介绍W25Q128时谈到W25Q128 Flash的扇区大小为4096，本示例我们要将elmfat文件系统挂载到W25Q128 Flash上，所以需要将maximum sector size修改为4096，然后保存配置退出。

本示例目的是在W25Q128 Flash上挂载elmfat文件系统，在挂载前需要先在W25Q128 Flash上创建elmfat格式的文件系统，挂在成功后通过DFS POSIX接口获取该文件系统的统计信息。在挂载elmfat的路径“/”下新建目录“/user”，使用DFS POSIX接口打开、关闭文件，并往该文件中写入、读取数据。

前篇博客已经完成了使用SFUD框架管理W25Q128 Flash，并向 I / O设备管理层注册了访问W25Q128 Flash的接口函数集合，所以elmfat文件系统可以直接通过 I / O设备管理接口访问到W25Q128。在文件dfs_sample.c中新增elmfat示例代码如下：

```c
// projects\stm32l475_dfs_sample\applications\dfs_sample.c

#include "drv_qspi.h"
#include "spi_flash_sfud.h"

#define QSPI_BUD_NAME       "qspi1"
#define QSPI_DEVICE_NAME    "qspi10"
#define W25Q_FLASH_NAME     "W25Q128"

#define QSPI_CS_PIN         GET_PIN(E, 11)

static int rt_hw_spi_flash_init(void)
{
    if(stm32_qspi_bus_attach_device(QSPI_BUD_NAME, QSPI_DEVICE_NAME, (rt_uint32_t)QSPI_CS_PIN, 1, RT_NULL, RT_NULL) != RT_EOK)
        return -RT_ERROR;

    if(rt_sfud_flash_probe(W25Q_FLASH_NAME, QSPI_DEVICE_NAME) == RT_NULL)
        return -RT_ERROR;

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);

static void elmfat_sample(void)
{
    int fd, size;
    struct statfs elm_stat;
    char str[] = "elmfat mount to W25Q flash.", buf[80];

    if(dfs_mkfs("elm", W25Q_FLASH_NAME) == 0)
        rt_kprintf("make elmfat filesystem success.\n");

    if(dfs_mount(W25Q_FLASH_NAME, "/", "elm", 0, 0) == 0)
        rt_kprintf("elmfat filesystem mount success.\n");

    if(statfs("/", &elm_stat) == 0)
        rt_kprintf("elmfat filesystem block size: %d, total blocks: %d, free blocks: %d.\n", 
                    elm_stat.f_bsize, elm_stat.f_blocks, elm_stat.f_bfree);

    if(mkdir("/user", 0x777) == 0)
        rt_kprintf("make a directory: '/user'.\n");

    rt_kprintf("Write string '%s' to /user/test.txt.\n", str);

    /* 以创建和读写模式打开文件，如果该文件不存在则创建该文件*/
    fd = open("/user/test.txt", O_WRONLY | O_CREAT);
    if (fd >= 0)
    {
        if(write(fd, str, sizeof(str)) == sizeof(str))
            rt_kprintf("Write data done.\n");

        close(fd);   
    }

    /* 以只读模式打开文件 */
    fd = open("/user/test.txt", O_RDONLY);
    if (fd >= 0)
    {
        size = read(fd, buf, sizeof(buf));

        close(fd);

        if(size == sizeof(str))
            rt_kprintf("Read data from file test.txt(size: %d): %s \n", size, buf);
    }
}
MSH_CMD_EXPORT(elmfat_sample, elmfat sample);
```
在env环境中执行scons --target=mdk5生成MDK5工程代码，打开project.uvprojx，将编译器配置为ARM Compiler V5，编译无报错。将程序烧录到STM32L475潘多拉开发板上，运行结果如下：
![elmfat示例运行结果](https://img-blog.csdnimg.cn/20191022200228756.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L20wXzM3NjIxMDc4,size_16,color_FFFFFF,t_70)
通过自动初始化组件配置了QSPI和SFUD，将elmfat文件系统挂载到W25Q128 Flash上的路径"/"成功，并通过DFS POSIX接口访问目录和文件正常。

DFS提供的finsh命令跟Linux访问目录和文件的命令类似，部分命令的使用也在上图给出了示范。



# 更多文章：

 - 《[IOT-OS之RT-Thread（十一）--- FAL分区管理与easyflash变量管理](https://blog.csdn.net/m0_37621078/article/details/102689903)》
 - 《[IOT-OS之RT-Thread（九）--- SPI设备对象管理与SFUD管理框架](https://blog.csdn.net/m0_37621078/article/details/102559086)》
 - 《[STM32之CubeL4（三）--- SPI + QSPI + HAL](https://blog.csdn.net/m0_37621078/article/details/101395150)》
 - 《[IOT-OS之RT-Thread（八）--- IIC设备对象管理与Sensor框架管理](https://blog.csdn.net/m0_37621078/article/details/103115383)》
 - 《[IOT-OS之RT-Thread（七）--- 设备模型框架与PIN设备对象管理](https://blog.csdn.net/m0_37621078/article/details/101158817)》
 - 《[IOT-OS之RT-Thread（六）--- 线程间同步与线程间通信](https://blog.csdn.net/m0_37621078/article/details/101082972)》
 - 《[IOT-OS之RT-Thread（五）--- 线程调度器与线程对象管理](https://blog.csdn.net/m0_37621078/article/details/100945020)》
 - 《[IOT-OS之RT-Thread（四）--- 时钟管理与内存管理](https://blog.csdn.net/m0_37621078/article/details/100859611)》
 - 《[IOT-OS之RT-Thread（三）--- C语言对象化与内核对象管理](https://blog.csdn.net/m0_37621078/article/details/100788959)》
 - 《[IOT-OS之RT-Thread（二）--- CPU架构与BSP移植过程](https://blog.csdn.net/m0_37621078/article/details/100715601)》
 - 《[IOT-OS之RT-Thread（一）---系统启动与初始化过程](https://blog.csdn.net/m0_37621078/article/details/100584591)》



