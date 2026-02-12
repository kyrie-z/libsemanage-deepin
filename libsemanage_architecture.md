# libsemanage 架构文档

## 目录
1. [项目概述](#项目概述)
2. [整体架构图](#整体架构图)
3. [核心组件类图](#核心组件类图)
4. [关键接口说明](#关键接口说明)
5. [数据流图](#数据流图)

---

## 项目概述

libsemanage（SELinux管理库）是SELinux策略管理框架的核心组成部分，提供了对SELinux策略的动态管理和修改能力。该库支持策略模块的安装、删除、启用/禁用，以及对用户、端口、接口、布尔值、文件上下文等策略对象的增删改查操作。

### 主要特性
- 模块化策略管理
- 事务性操作（Transaction机制）
- 本地和策略数据库分离
- 支持多种存储后端（当前支持Direct模式）
- 提供C、Python、Ruby等多种语言绑定

---

## 整体架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         应用层 (Applications)                    │
│  (semanage命令行工具, setsebool, semanage Python/Ruby绑定等)      │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                        公共API层                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │ handle.h     │  │ modules.h    │  │ record_*.h           │  │
│  │ 句柄管理     │  │ 模块管理     │  │ 记录类型定义         │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                        中间抽象层                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                  数据库抽象层 (Database Layer)           │  │
│  │  ┌────────────┐  ┌────────────┐  ┌──────────────────┐  │  │
│  │  │ Local DB   │  │ Policy DB  │  │ Active DB        │  │  │
│  │  │ 本地修改   │  │ 策略数据库  │  │ 内核活动策略     │  │  │
│  │  └────────────┘  └────────────┘  └──────────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  dbase_table_t: 统一的数据库操作接口                             │
│  - add, modify, set, del, query, exists, count, iterate, list   │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                        记录抽象层                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │ record_table │  │ 各种Record   │  │ Key提取和比较        │  │
│  │ 记录接口     │  │ 实现         │  │                      │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
│                                                                 │
│  支持的记录类型:                                                 │
│  - Boolean（布尔值）                                             │
│  - User（用户）                                                  │
│  - Port（端口）                                                  │
│  - Interface（网络接口）                                         │
│  - Node（网络节点）                                              │
│  - Fcontext（文件上下文）                                        │
│  - Seuser（SELinux用户）                                         │
│  - IBPKey/IBEndPort（InfiniBand支持）                            │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                          存储层                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                  Direct API (direct_api.c)               │  │
│  │                                                          │  │
│  │  ┌─────────────────────────────────────────────────┐    │  │
│  │  │         Store Management (semanage_store)        │    │  │
│  │  │  - 模块存储管理                                  │    │  │
│  │  │  - 沙箱创建和管理                                │    │  │
│  │  │  - 策略编译和链接                                │    │  │
│  │  │  - 文件系统操作                                  │    │  │
│  │  └─────────────────────────────────────────────────┘    │  │
│  │                                                          │  │
│  │  ┌─────────────────────────────────────────────────┐    │  │
│  │  │         Lock Management                          │    │  │
│  │  │  - 事务锁 (translock)                            │    │  │
│  │  │  - 活动锁 (activelock)                           │    │  │
│  │  └─────────────────────────────────────────────────┘    │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                     底层存储                                    │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────────────┐  │
│  │ 模块文件   │  │ 配置文件   │  │ 沙箱目录结构             │  │
│  │ .pp/.cil   │  │ semanage.  │  │ ┌────────────────────┐  │  │
│  │            │  │ conf       │  │ │ modules/           │  │  │
│  ├────────────┤  └────────────┘  │ ├── linked/          │  │  │
│  │ 数据库文件 │                  │ └── policy.xx/       │  │  │
│  │ *.local    │                  └────────────────────┘  │  │
│  └────────────┘                                            │  │
└───────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                     外部依赖库                                   │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────────────┐  │
│  │ libsepol   │  │ libselinux │  │ CIL (Common            │  │
│  │ 策略处理   │  │ SELinux    │  │ Intermediate Language) │  │
│  │            │  │ 运行时库   │  │                          │  │
│  └────────────┘  └────────────┘  └──────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

---

## 核心组件类图

### 1. Handle管理组件

```
┌─────────────────────────────────────────────────────────────┐
│                     semanage_handle_t                        │
├─────────────────────────────────────────────────────────────┤
│ - con_id: int                                                │
│ - is_connected: int                                          │
│ - is_in_transaction: int                                     │
│ - do_reload: int                                             │
│ - do_rebuild: int                                            │
│ - check_ext_changes: int                                     │
│ - modules_modified: int                                      │
│ - timeout: int                                               │
│ - priority: uint16_t                                        │
│ - sepolh: sepol_handle_t*                                   │
│ - conf: semanage_conf_t*                                    │
│ - funcs: semanage_policy_table*                              │
│ - dbase[24]: dbase_config_t                                  │
├─────────────────────────────────────────────────────────────┤
│ + semanage_handle_create()                                   │
│ + semanage_handle_destroy()                                  │
│ + semanage_connect()                                         │
│ + semanage_disconnect()                                      │
│ + semanage_begin_transaction()                               │
│ + semanage_commit()                                         │
│ + semanage_set_reload()                                      │
│ + semanage_set_rebuild()                                     │
│ + semanage_set_default_priority()                            │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ 关联
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                 semanage_conf_t (配置)                        │
├─────────────────────────────────────────────────────────────┤
│ - store_path: char*                                          │
│ - store_root_path: char*                                     │
│ - store_type: enum                                           │
│ - compiler_directory_path: char*                             │
│ - ignore_module_cache: int                                   │
└─────────────────────────────────────────────────────────────┘
```

### 2. 数据库抽象层次结构

```
┌─────────────────────────────────────────────────────────────┐
│                     dbase_table_t (接口)                     │
├─────────────────────────────────────────────────────────────┤
│ + add()        : 添加记录                                    │
│ + modify()     : 添加或修改记录                               │
│ + set()        : 仅修改已存在记录                              │
│ + del()        : 删除记录                                    │
│ + query()      : 查询记录                                    │
│ + exists()     : 检查记录是否存在                              │
│ + count()      : 统计记录数量                                 │
│ + iterate()    : 遍历记录                                    │
│ + list()       : 获取记录列表                                 │
│ + cache()      : 缓存数据库                                  │
│ + drop_cache() : 丢弃缓存                                    │
│ + is_modified(): 检查是否已修改                               │
│ + flush()      : 将修改刷新到后端                             │
│ + get_rtable() : 获取记录表                                 │
└──────────────────┬──────────────────────────────────────────┘
                   │ 实现
                   ├─────────────────┬─────────────────┬───────────┐
                   ▼                 ▼                 ▼           ▼
┌──────────────────┐ ┌──────────────┐ ┌─────────────┐ ┌──────────┐
│ database_file_t  │ │ database_llist│ │database_    │ │database_ │
│ 文件存储实现     │ │ 链表实现     │ │ policydb_t   │ │join_t    │
│                  │ │              │ │策略数据库    │ │联合查询  │
│ - *_file.c       │ │              │ │             │ │          │
└──────────────────┘ └──────────────┘ └─────────────┘ └──────────┘
```

### 3. 记录抽象层次结构

```
┌─────────────────────────────────────────────────────────────┐
│                    record_table_t (接口)                     │
├─────────────────────────────────────────────────────────────┤
│ + create()      : 创建记录                                  │
│ + key_extract() : 提取键                                    │
│ + key_free()    : 释放键                                    │
│ + compare()     : 比较记录与键                              │
│ + compare2()    : 比较两个记录                              │
│ + clone()       : 克隆记录                                  │
│ + free()        : 释放记录                                  │
└──────────────────┬──────────────────────────────────────────┘
                   │ 实现
                   ├─────────────────┬───────────┬──────────────┐
                   ▼                 ▼           ▼              ▼
┌──────────────────┐ ┌────────────┐ ┌────────┐ ┌────────────┐
│ bool_record_t    │ │ user_      │ │ port_  │ │ fcontext_  │
│ 布尔值记录        │ │ record_t   │ │ record │ │ record_t   │
├──────────────────┤ └────────────┘ └────────┘ └────────────┘
│ - name: char*    │
│ - value: int     │
├──────────────────┤
│ + bool_create()  │
│ + bool_set_name()│
│ + bool_get_value()│
└──────────────────┘
```

### 4. 模块管理组件

```
┌─────────────────────────────────────────────────────────────┐
│                 semanage_module_info_t                       │
├─────────────────────────────────────────────────────────────┤
│ - priority: uint16_t                                        │
│ - name: char*                                                │
│ - lang_ext: char*                                            │
│ - enabled: int                                               │
├─────────────────────────────────────────────────────────────┤
│ + semanage_module_install()      : 安装模块                   │
│ + semanage_module_remove()       : 删除模块                   │
│ + semanage_module_extract()      : 提取模块                   │
│ + semanage_module_list()         : 列出模块                   │
│ + semanage_module_set_enabled()  : 设置启用状态               │
│ + semanage_module_get_enabled()  : 获取启用状态               │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ 用于标识
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                 semanage_module_key_t                        │
├─────────────────────────────────────────────────────────────┤
│ - priority: uint16_t                                        │
│ - name: char*                                                │
└─────────────────────────────────────────────────────────────┘
```

---

## 关键接口说明

### 一、Handle管理接口（handle.h）

#### 核心创建与销毁

**`semanage_handle_t *semanage_handle_create(void)`**
- **功能**: 创建并初始化一个semanage句柄
- **说明**: 
  - 这是使用libsemanage的第一步
  - 初始化时会读取配置文件（/etc/selinux/semanage.conf）
  - 默认优先级为400
  - 默认会在commit后重新加载策略
- **返回值**: 成功返回句柄指针，失败返回NULL

**`void semanage_handle_destroy(semanage_handle_t *handle)`**
- **功能**: 销毁semanage句柄并释放所有相关资源
- **说明**: 
  - 调用前应确保已经断开连接
  - 会释放配置、sepol句柄等所有资源

#### 连接管理

**`int semanage_connect(semanage_handle_t *handle)`**
- **功能**: 连接到策略管理后端
- **说明**: 
  - 支持的连接类型：SEMANAGE_CON_DIRECT
  - 直接模式连接到本地文件系统策略存储
  - 返回0表示成功，负值表示失败
- **返回值**: 0成功，<0失败

**`int semanage_disconnect(semanage_handle_t *handle)`**
- **功能**: 断开后端连接
- **说明**: 
  - 会清理连接相关资源
  - 如果未连接则无操作

#### 事务管理

**`int semanage_begin_transaction(semanage_handle_t *handle)`**
- **功能**: 开始一个事务
- **说明**: 
  - 获取事务锁，防止并发修改
  - 设置超时时间由handle->timeout控制
  - -1表示无限等待，0表示立即返回
  - 所有修改操作必须在事务内进行
- **返回值**: 0成功，<0失败

**`int semanage_commit(semanage_handle_t *handle)`**
- **功能**: 提交事务并保存所有更改
- **说明**: 
  - 如果配置了do_reload，会重新加载策略
  - 如果配置了do_rebuild，会强制重建策略
  - 释放事务锁
  - 返回策略序列号或错误码
- **返回值**: >=0策略序列号，<0失败

**配置选项设置**

- `semanage_set_reload()`: 设置提交后是否重新加载策略
- `semanage_set_rebuild()`: 设置是否强制重建策略
- `semanage_set_check_ext_changes()`: 设置是否检查外部变化
- `semanage_set_create_store()`: 设置是否自动创建存储目录
- `semanage_set_store_root()`: 设置存储根路径
- `semanage_set_default_priority()`: 设置默认优先级（100-999）

---

### 二、模块管理接口（modules.h）

#### 模块安装与删除

**`int semanage_module_install(semanage_handle_t *sh, char *module_data, size_t data_len, const char *name, const char *ext_lang)`**
- **功能**: 安装一个新模块
- **参数**:
  - `module_data`: 模块数据缓冲区（支持bzip压缩）
  - `data_len`: 数据长度
  - `name`: 模块名称
  - `ext_lang`: 语言扩展（如"cil"、"te"）
- **说明**: 
  - 使用handle中设置的优先级
  - 模块数据会被写入模块存储目录
- **返回值**: 0成功，<0失败

**`int semanage_module_install_file(semanage_handle_t *sh, const char *module_name)`**
- **功能**: 从文件安装模块
- **参数**: `module_name`: 模块文件路径
- **说明**: 方便函数，自动读取文件并调用install

**`int semanage_module_remove(semanage_handle_t *sh, char *module_name)`**
- **功能**: 删除指定模块
- **说明**: 支持使用模块键进行精确删除

#### 模块查询

**`int semanage_module_list(semanage_handle_t *sh, semanage_module_info_t **modinfos, int *num_modules)`**
- **功能**: 列出所有模块
- **参数**:
  - `modinfos`: 输出参数，模块信息数组
  - `num_modules`: 输出参数，模块数量
- **说明**: 
  - 按优先级和字母顺序排序
  - 调用者需要用semanage_module_info_datum_destroy释放
- **返回值**: 0成功，<0失败

**`int semanage_module_extract(semanage_handle_t *sh, semanage_module_key_t *modkey, int extract_cil, void **mapped_data, size_t *data_len, semanage_module_info_t **modinfo)`**
- **功能**: 提取模块数据
- **参数**:
  - `modkey`: 模块键
  - `extract_cil`: 0提取原始数据，1提取CIL格式
  - `mapped_data`: 输出的映射数据
  - `data_len`: 数据长度
  - `modinfo`: 输出的模块信息
- **说明**: 
  - 数据使用mmap映射，调用者需用munmap释放
- **返回值**: 0成功，<0失败

#### 模块启用管理

**`int semanage_module_set_enabled(semanage_handle_t *sh, const semanage_module_key_t *modkey, int enabled)`**
- **功能**: 设置模块启用/禁用状态
- **参数**:
  - `modkey`: 模块键（只需设置name）
  - `enabled`: 1启用，0禁用
- **说明**: 
  - 启用状态按模块名称全局控制
  - 所有优先级的同名模块会一起启用/禁用

---

### 三、数据库操作接口

所有数据库对象都遵循统一的接口模式，以布尔值为例：

#### Local操作接口（本地修改）

**`int semanage_bool_modify_local(semanage_handle_t *handle, const semanage_bool_key_t *key, const semanage_bool_t *data)`**
- **功能**: 修改或添加本地布尔值
- **类型**: modify (如果不存在则添加，存在则替换)
- **说明**: 修改只影响本地数据库，需commit才会生效

**`int semanage_bool_set_local(semanage_handle_t *handle, const semanage_bool_key_t *key, const semanage_bool_t *data)`**
- **功能**: 设置本地布尔值（如果不存在则失败）
- **类型**: set (仅修改已存在的项)

**`int semanage_bool_del_local(semanage_handle_t *handle, const semanage_bool_key_t *key)`**
- **功能**: 删除本地布尔值

**`int semanage_bool_query_local(semanage_handle_t *handle, const semanage_bool_key_t *key, semanage_bool_t **response)`**
- **功能**: 查询本地布尔值
- **说明**: 返回的记录属于调用者，需要手动释放

**`int semanage_bool_iterate_local(semanage_handle_t *handle, int (*handler)(const semanage_bool_t *record, void *varg), void *handler_arg)`**
- **功能**: 遍历所有本地布尔值
- **说明**: handler返回1停止，-1出错，0继续

**`int semanage_bool_list_local(semanage_handle_t *handle, semanage_bool_t ***records, unsigned int *count)`**
- **功能**: 获取所有本地布尔值列表

#### Policy操作接口（策略+本地）

API形式与Local相同，只是函数名中的`_local`替换为`_policy`：
- `semanage_bool_modify_policy()`
- `semanage_bool_query_policy()`
- `semanage_bool_list_policy()`

**区别**: Policy视图包含基础策略和本地修改

#### Active操作接口（活动内核策略）

仅布尔值支持Active视图：
- `semanage_bool_exists_active()`
- `semanage_bool_query_active()`
- `semanage_bool_count_active()`
- `semanage_bool_iterate_active()`
- `semanage_bool_list_active()`

**说明**: 提供对内核中当前活动策略的直接访问

---

### 四、记录操作接口（以Boolean为例）

#### 记录创建与销毁

**`int semanage_bool_create(semanage_handle_t *handle, semanage_bool_t **bool_ptr)`**
- **功能**: 创建一个空的布尔值记录
- **返回值**: 0成功，<0失败

**`void semanage_bool_free(semanage_bool_t *boolean)`**
- **功能**: 释放布尔值记录及其占用的资源

**`int semanage_bool_clone(semanage_handle_t *handle, const semanage_bool_t *boolean, semanage_bool_t **bool_ptr)`**
- **功能**: 深度克隆布尔值记录

#### 键操作

**`int semanage_bool_key_create(semanage_handle_t *handle, const char *name, semanage_bool_key_t **key)`**
- **功能**: 创建一个用于标识布尔值的键
- **参数**: `name`: 布尔值名称
- **说明**: 键用于数据库操作中的查询、删除等

**`void semanage_bool_key_free(semanage_bool_key_t *key)`**
- **功能**: 释放键资源

#### 记录属性访问

**`const char *semanage_bool_get_name(const semanage_bool_t *boolean)`**
- **功能**: 获取布尔值名称
- **返回值**: 名称字符串（不应释放）

**`int semanage_bool_set_name(semanage_handle_t *handle, semanage_bool_t *boolean, const char *name)`**
- **功能**: 设置布尔值名称

**`int semanage_bool_get_value(const semanage_bool_t *boolean)`**
- **功能**: 获取布尔值（0或1）

**`void semanage_bool_set_value(semanage_bool_t *boolean, int value)`**
- **功能**: 设置布尔值

---

### 五、存储管理接口（semanage_store.h - 内部接口）

#### 沙箱管理

**`int semanage_make_sandbox(semanage_handle_t *sh)`**
- **功能**: 创建沙箱目录结构
- **说明**: 
  - 沙箱是事务的工作区
  - 包含modules、linked、policy等子目录
  - 支持多个沙箱：ACTIVE、PREVIOUS、TMP

**`int semanage_install_sandbox(semanage_handle_t *sh)`**
- **功能**: 将沙箱内容安装到系统
- **说明**: 
  - 原子性操作
  - 将TMP沙箱移动到ACTIVE位置
  - 同步文件系统

#### 锁管理

**`int semanage_get_trans_lock(semanage_handle_t *sh)`**
- **功能**: 获取事务锁
- **说明**: 
  - 防止并发修改
  - 使用文件锁实现
  - 支持超时

**`int semanage_get_active_lock(semanage_handle_t *sh)`**
- **功能**: 获取活动锁
- **说明**: 防止读取修改中的数据

**`void semanage_release_trans_lock(semanage_handle_t *sh)`**
- **功能**: 释放事务锁

#### 策略数据库操作

**`int semanage_read_policydb(semanage_handle_t *sh, sepol_policydb_t *policydb, enum semanage_sandbox_defs file)`**
- **功能**: 从文件读取策略数据库
- **说明**: 使用libsepol解析二进制策略文件

**`int semanage_write_policydb(semanage_handle_t *sh, sepol_policydb_t *policydb, enum semanage_sandbox_defs file)`**
- **功能**: 写入策略数据库到文件

#### CIL模块处理

**`int semanage_load_files(semanage_handle_t *sh, cil_db_t *cildb, char **filenames, int num_modules)`**
- **功能**: 加载CIL模块文件
- **说明**: 使用CIL编译器处理模块

**`int semanage_get_cil_paths(semanage_handle_t *sh, semanage_module_info_t *modinfos, int len, char ***filenames)`**
- **功能**: 获取所有CIL模块文件路径
- **说明**: 按优先级和名称排序

#### 验证操作

**`int semanage_verify_modules(semanage_handle_t *sh, char **module_filenames, int num_modules)`**
- **功能**: 验证模块文件
- **说明**: 语法和语义检查

**`int semanage_verify_linked(semanage_handle_t *sh)`**
- **功能**: 验证链接后的策略

**`int semanage_verify_kernel(semanage_handle_t *sh)`**
- **功能**: 验证内核策略的有效性

---

## 数据流图

### 典型操作流程：修改布尔值

```
用户程序
   │
   ├─> 1. semanage_handle_create()  // 创建句柄
   │
   ├─> 2. semanage_connect()       // 连接到存储
   │
   ├─> 3. semanage_begin_transaction()  // 开始事务
   │
   ├─> 4. semanage_bool_create()    // 创建记录
   │
   ├─> 5. semanage_bool_set_name()
   │
   ├─> 6. semanage_bool_set_value()
   │
   ├─> 7. semanage_bool_key_create() // 创建键
   │
   ├─> 8. semanage_bool_modify_local()  // 修改本地数据库
   │       │
   │       ▼
   │    [数据库抽象层]
   │       │
   │       ▼
   │    [record操作层]
   │
   ├─> 9. semanage_commit()        // 提交事务
   │       │
   │       ├─> 收集所有修改
   │       │
   │       ├─> 创建沙箱 (semanage_make_sandbox)
   │       │
   │       ├─> 编译模块 (使用CIL compiler)
   │       │
   │       ├─> 链接策略 (libsepol)
   │       │
   │       ├─> 生成二进制策略
   │       │
   │       ├─> 验证策略
   │       │
   │       ├─> 安装沙箱 (semanage_install_sandbox)
   │       │
   │       └─> 重新加载策略 (如果配置)
   │
   ├─> 10. semanage_disconnect()   // 断开连接
   │
   └─> 11. semanage_handle_destroy() // 销毁句柄
```

### 模块安装流程

```
用户程序
   │
   ├─> semanage_module_install()
   │       │
   │       ├─> 解析模块数据
   │       │
   │       ├─> 验证模块格式
   │       │
   │       ├─> 写入模块存储目录
   │       │    路径: <store_root>/active/modules/<priority>/<name>
   │       │
   │       ├─> 更新模块索引
   │       │
   │       └─> 标记modules_modified
   │
   └─> semanage_commit()
           │
           ├─> 重建模块列表
           │
           ├─> 编译新添加的模块
           │
           ├─> 链接所有模块
           │
           └─> 生成新策略
```

### 三层数据库视图关系

```
内核运行时
    │
    ▼
┌─────────────────┐
│ Active DB       │  ← 当前内核加载的策略
│ (实时)          │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Policy DB       │  ← 基础策略 + 本地修改的合并视图
│ (只读视图)      │     = Base Policy + Local Modifications
└────────┬────────┘
         │
         ├──────────────┐
         │              │
         ▼              ▼
┌─────────────────┐  ┌─────────────────┐
│ Base Policy     │  │ Local DB        │  ← 用户自定义的修改
│ (基础策略)       │  │ (可写)          │     存储在*.local文件
└─────────────────┘  └─────────────────┘

查询流程:
1. 查询Active: 直接读取内核 -> 返回运行时值
2. 查询Policy: 合并Base + Local -> 返回提交后的预期值
3. 查询Local: 仅读取Local DB -> 返回未提交的本地修改
```

---

## 数据库索引说明

semanage_handle_t维护24个数据库配置，分为三类：

### Local数据库 (索引0-10)
- **作用**: 存储用户未提交的自定义修改
- **存储位置**: *.local 文件
- **特点**: 可读写，修改在commit后才生效
- **包含**:
  - DBASE_LOCAL_USERS_BASE: 用户基础信息
  - DBASE_LOCAL_USERS_EXTRA: 用户扩展信息
  - DBASE_LOCAL_USERS: 完整用户信息
  - DBASE_LOCAL_PORTS: 端口配置
  - DBASE_LOCAL_INTERFACES: 网络接口
  - DBASE_LOCAL_BOOLEANS: 布尔值
  - DBASE_LOCAL_FCONTEXTS: 文件上下文
  - DBASE_LOCAL_SEUSERS: SELinux用户
  - DBASE_LOCAL_NODES: 网络节点
  - DBASE_LOCAL_IBPKEYS: InfiniBand分区密钥
  - DBASE_LOCAL_IBENDPORTS: InfiniBand端点端口

### Policy数据库 (索引11-22)
- **作用**: 基础策略与本地修改的合并视图
- **存储**: 虚拟视图，通过合并Base和Local生成
- **特点**: 只读查询，反映commit后的最终状态
- **包含**: 与Local对应的10个数据库，外加fcontext_homedirs

### Active数据库 (索引23)
- **作用**: 内核中当前加载的活动策略
- **存储**: 直接读取内核策略
- **特点**: 仅支持布尔值，反映运行时状态
- **包含**: DBASE_ACTIVE_BOOLEANS

---

## 文件系统布局

```
/etc/selinux/
├── semanage.conf              # libsemanage配置文件
│
└── <policy_name>/              # 策略名称目录（如targeted）
    ├── active/                 # 活动沙箱
    │   ├── modules/
    │   │   ├── 100/            # 优先级100的模块
    │   │   ├── 200/
    │   │   └── 400/            # 默认优先级
    │   ├── linked/             # 链接后的CIL文件
    │   ├── policy.31           # 二进制策略文件
    │   ├── file_contexts       # 文件上下文
    │   ├── file_contexts.homedirs
    │   ├── file_contexts.local # 本地文件上下文
    │   ├── booleans.local      # 本地布尔值
    │   ├── ports.local         # 本地端口配置
    │   ├── interfaces.local    # 本地接口配置
    │   └── nodes.local         # 本地节点配置
    │
    ├── previous/               # 前一次的策略版本（备份）
    │
    └── modules/                # 模块源文件存储（根据不同优先级存储）
        └── active/             # 激活的模块（按优先级和模块名组织）
```

---

## 配置文件说明

### /etc/selinux/semanage.conf

```ini
# 模块存储根路径
module-store = active

# HLL编译器目录（默认为 /usr/libexec/selinux/hll）
compiler-directory = /usr/libexec/selinux/hll

# 是否忽略模块缓存
ignore-module-cache = false

# 默认优先级（代码中默认为400）
# 优先级范围：100-999
# 优先级越高，模块越晚加载（可以覆盖低优先级的规则）

# 是否禁用dontaudit规则
# disable-dontaudit = true
```

---

## 关键设计模式

### 1. 策略模式（Strategy Pattern）
- **实现**: `semanage_policy_table` 函数指针表
- **用途**: 支持多种后端连接类型（Direct、PolServer等）
- **优点**: 添加新后端无需修改客户端代码

### 2. 工厂模式（Factory Pattern）
- **实现**: 各record的create函数
- **用途**: 统一创建各种记录对象

### 3. 模板方法模式（Template Method Pattern）
- **实现**: 数据库操作的统一接口
- **用途**: 各种数据库实现遵循相同操作流程

### 4. 适配器模式（Adapter Pattern）
- **实现**: 不同层次数据库的适配
- **用途**: 将libsepol的policydb适配为数据库接口

---

## 扩展和支持的记录类型总结

libsemanage支持以下策略对象类型的管理：

| 类型 | 记录结构 | 数据库文件 | 说明 |
|------|---------|-----------|------|
| Boolean | semanage_bool_t | booleans.local | SELinux布尔值开关 |
| User | semanage_user_t | users.local | SELinux用户角色映射 |
| Port | semanage_port_t | ports.local | 端口号的安全上下文 |
| Interface | semanage_iface_t | interfaces.local | 网络接口的安全上下文 |
| Node | semanage_node_t | nodes.local | 网络节点的安全上下文 |
| Fcontext | semanage_fcontext_t | file_contexts.local | 文件路径的安全上下文 |
| Seuser | semanage_seuser_t | seusers.local | Linux用户到SELinux用户的映射 |
| IBPKey | semanage_ibpkey_t | ibpkeys.local | InfiniBand分区密钥 |
| IBEndPort | semanage_ibendport_t | ibendports.local | InfiniBand端点端口 |

每种类型都提供：
- Record操作：创建、克隆、销毁、属性访问
- Local数据库：修改、查询、删除本地配置
- Policy数据库：查询合并后的策略配置
- Active数据库（仅Boolean）：查询内核运行时状态

---

## 最佳实践建议

1. **总是使用事务**
   - 修改前调用`semanage_begin_transaction()`
   - 完成后调用`semanage_commit()`
   - 避免在事务外修改

2. **正确处理错误**
   - 检查所有函数的返回值
   - 使用`semanage_msg_get_level()`和`semanage_msg_get_channel()`获取错误详情

3. **资源管理**
   - 创建的记录和键必须手动释放
   - 列表查询返回的数组需要逐个释放
   - 句柄最终必须销毁

4. **优先级使用**
   - 系统策略：100-199
   - 发行版策略：200-399
   - 安装时：400-599
   - 用户自定义：600-999

5. **性能考虑**
   - 批量操作：尽量在单个事务中完成
   - 缓存复用：在需要时调用cache()刷新
   - 避免频繁的事务边界

---

## 总结

libsemanage提供了层次清晰、接口统一的SELinux策略管理框架：

1. **Handle层**: 统一的会话和事务管理
2. **API层**: 面向应用的高层接口
3. **数据库抽象层**: 统一的数据操作接口
4. **记录抽象层**: 统一的记录操作接口
5. **存储层**: 文件系统存储和沙箱管理

这种分层设计使得：
- 新增策略对象类型只需实现record和database接口
- 新增存储后端只需实现policy函数表
- 客户端代码简洁统一
- 易于测试和维护

通过本架构文档，开发者应该能够：
- 理解libsemanage的整体设计
- 掌握关键接口的使用方法
- 知道如何扩展新功能
- 理解数据流和事务机制