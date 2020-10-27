#include <bits/stdc++.h>

using namespace std;

//用户相关
#define MAXUSERNUM 30 //系统最多有30的用户可使用
#define USERNUM 8     //最多允许8个用户登录
#define FILESYS 100   //系统自身的“用户ID”
#define ONLYREAD 1    //用户类别：只读用户
#define CANWRITE 2    //用户类别：可写用户
#define ADMIN 3       //用户类别：管理员用户
#define NOFILE 20     //每个用户最多可打开20个文件，即用户打开文件最大次数
#define PWDSIZ 6      //用户口令字最大长度

//磁盘中信息相关
#define BLOCKSIZ 512 //磁盘上每一块大小
#define NICFREE 50   //磁盘上超级块中空闲块数组的最大块数
#define NICINOD 50   //磁盘上超级块中空闲节点的最大块数
#define ZEROFLAG 'Z' //初始化时的超级块修改标志

#define DINODESIZ 32 //每个磁盘i节点所占字节
#define DINODEBLK 32 //所有磁盘i节点共占32个物理块
#define IFREE 50     //组长块所在块中空闲i节点数组的最大节点数
#define ILEADNUM 11  //设定11个索引组长区，存放在文件区，共需3个数据块，索引节点区只存放索引节点
#define DIRFILE 1    //目录文件
#define DATAFILE 2   //数据文件
#define DIREAD 1     //只读文件
#define DIWRITE 2    //可改文件
#define DISYS 3      //系统文件

#define FILEBLK 512    //共有512个目录、文件物理块
#define DISKFULL 65535 //磁盘已满的返回值
#define DIRITEMNUM 128 //每个目录所包含的最大目录项数（文件数）
#define DIRSIZ 14      //每个目录项名字部分所占字节数
#define DATABLK 500    //设定500个文件物理块
#define DATAFREE 50    //组长块所在组中空闲块数组的最大块数
#define DATAMEMBLK 490 //设定490个文件组员块
#define DATALEADBLK 10 //设定10个文件组长块

//内存中信息相关
#define SYSOPENFILE 40 //系统打开文件表，表中最大项数
#define FILEEMPTY 0    //系统打开文件表，表项为空/表项未分配的文件标志
#define FILEOPEN 1     //系统打开文件表，文件仅打开的文件标志
#define FILECHANGE 2   //系统打开文件表，文件修改未保存的文件标志
#define FILESAVE 3     //系统打开文件表，文件修改且保存的文件标志
#define IZERO 0        //内存i节点表，初始化时空i节点的标识号
#define IEMPTY 0       //内存i节点表，i节点项为空/i节点项未分配的标志
#define IOPEN 1        //内存i节点表，i节点仅打开的标志
#define ICHANGE 2      //内存i节点表，i节点被修改的标志
#define ILOCK 3        //内存i节点表，i节点被锁的标志
#define NHINO 128      //共128个Hash链表，提供索引i节点（必须为2的幂）
#define NADDR 10       //每个i节点最多指向10块，addr[0] ~addr[9]
#define READ 1         //用户读操作
#define WRITE 2        //用户写操作
#define DELETE 3       //用户删除操作

typedef struct
{
    unsigned short u_category; //用户类别
    unsigned short u_uid;      //用户ID
    unsigned short u_gid;      //组ID
    string u_password;         //用户密码
    int u_logcount;            //用户的登录次数
} user;                        //用户结构

typedef struct
{
    unsigned short block_number;       //组长块的块号
    unsigned short free_count;         //组长块所在本组中剩余的空闲块块数
    unsigned int free_array[DATAFREE]; //组长块所在本组中的空闲块数组
} group_lead_block;                    //组长块的结构，单位长度204字节，一个数据块可存放两个组长块

typedef struct
{
    unsigned short s_i_size;              //i节点块数
    unsigned short s_free_i_size;         //空闲i节点数
    unsigned int s_free_i_array[NICINOD]; //空闲i节点数组
    unsigned int s_free_i_pointer;        //空闲i节点指针
    unsigned int s_i_remember_node;       //铭记i节点

    unsigned int s_data_size;                //数据块块数
    unsigned int s_free_data_size;           //空闲数据块块数
    unsigned int s_free_data_array[NICFREE]; //空闲数据块数组
    unsigned short s_free_data_pointer;      //空闲数据块指针

    char s_change_flag; //超级块修改标志
} super_block;          //超级块的结构，单位长度424字节(根据字节补齐规则，423补齐至424)

typedef struct
{
    unsigned short di_file_number; //关联文件数
    unsigned short di_mode;        //存取权限
    unsigned short di_uid;         //用户ID
    unsigned short di_type;        //文件类型
    unsigned int di_size;          //文件大小
    unsigned short di_addr[NADDR]; //物理块号
} disk_i_node;                     //磁盘索引节点的结构，单位长度32字节

typedef struct
{
    string dir_item_name;       //目录名
    unsigned short dir_item_ID; //目录号，即为i节点号
} dir_items;                    //目录项的结构,单位长度16字节

typedef struct
{
    //引导块
    char g_block[BLOCKSIZ];
    //超级块
    super_block s_block;
    //磁盘索引节点区
    disk_i_node d_i_array[BLOCKSIZ * DINODEBLK / DINODESIZ]; //磁盘索引节点，共计512 * 32 / 32 = 512个磁盘索引节点
    //磁盘目录、文件数据块区
    group_lead_block leader_i_block[ILEADNUM];       //索引组长物理块
    group_lead_block leader_data_block[DATALEADBLK]; //文件组长物理块
    char member_data_block[DATAMEMBLK][BLOCKSIZ];    //文件组员物理块
} disk;                                              //磁盘的结构，单位长度274536字节

typedef struct i_node
{
    disk_i_node i_basic;   //来自磁盘索引节点的基本信息
    i_node *i_pointer1;    //i节点指针1
    i_node *i_pointer2;    //i节点指针2
    unsigned int i_number; //i节点标识
    unsigned int i_count;  //进程访问引用计数
    unsigned int i_mark;   //i节点标志，是否被锁或者修改
} i_node;                  //内存索引节点的结构

typedef struct
{
    unsigned short ftable_flag; //文件操作标志
    unsigned int ftable_count;  //引用计数
    i_node *ftable_i_node;      //指向内存i节点的指针
    unsigned int ftable_offset; //读/写指针
} open_file_table_item;         //系统打开文件表表项的结构

typedef struct
{
    user u_basic;
    vector<unsigned short> u_ftable; //用户打开文件表，每一项的一个数代表索引节点编号
} login_user_table_item;             //已登录用户信息的结构

typedef struct
{
    vector<dir_items> direct;   //目录表，即子目录项的数组
    unsigned short this_dir_id; //当前目录号
    string this_dir_name;       //当前目录名
} current_direct_table;         //目录的结构

typedef struct
{
    string f_name;                //文件名
    disk_i_node f_i_basic;        //文件基本描述信息
    vector<char *> f_data;        //文件的数据信息
    current_direct_table old_dir; //文件之前的目录
    unsigned short mode;          //剪切/复制的标志
} move_file_info;                 //剪切/复制时保存的文件信息

//函数声明
void write_info_to_disk();                                                  //写回磁盘文件
void read_info_from_disk();                                                 //从磁盘文件读
void disk_init();                                                           //磁盘格式化
void open_file_system();                                                    //打开文件系统
void exit_file_system();                                                    //关闭文件系统
void user_login();                                                          //用户登录
void user_logout();                                                         //用户注销
void user_switch();                                                         //用户切换
i_node *i_get(unsigned int dinodeid);                                       //获取内存i节点
void i_put(i_node *pinode);                                                 //释放内存i节点
i_node *i_alloc();                                                          //分配磁盘i节点
void i_free(unsigned int dinodeid);                                         //释放磁盘i节点
unsigned short block_alloc();                                               //分配磁盘块
void block_free(unsigned int block_num);                                    //释放磁盘块
unsigned short name_i(string name);                                         //在目录里查找某一文件
unsigned short i_name(string name, unsigned short ID);                      //分配文件目录项
bool access(i_node *inode, unsigned short mode);                            //检查控制权限
void print_cur_path();                                                      //输出当前用户所处目录路径
void show_dir();                                                            //展示当前目录
void make_dir(string d_name, unsigned short num);                           //创建目录
void delete_dir(string d_name);                                             //删除目录
void change_dir(unsigned short i_number, string new_name);                  //修改目录
void open_file(string filename);                                            //打开文件
void close_file(string filename);                                           //关闭文件
void create_file(string filename, unsigned short mode, unsigned short num); //创建文件
void delete_file(string filename);                                          //删除文件
void read_file(string f_name);                                              //读文件
void write_file(string filename, string type);                              //写文件
void cut_file(string f_name);                                               //剪切文件
void copy_file(string f_name);                                              //复制文件
void paste_file(string f_name);                                             //粘贴文件
void search_file_or_dir(string name);                                       //查找文件
void search_file_or_dir_core(string name, vector<i_node *> *result_i, vector<vector<current_direct_table>> *result_path);

//全局变量定义
vector<user> User(MAXUSERNUM); //保存用户信息
disk Disk;                     //保存磁盘信息

unordered_map<unsigned short, open_file_table_item> open_ftable; //系统打开文件表
vector<i_node> i_table;                                          //内存i节点表
vector<login_user_table_item> user_table;                        //已登录的用户信息表
login_user_table_item cur_user;                                  //当前正在操作的用户编号
current_direct_table cur_dir;                                    //当前所处的目录
vector<current_direct_table> last_dir_list;                      //当前目录的上级目录表
move_file_info temp_file;                                        //剪切/复制时临时保存的文件内容

void write_info_to_disk() //超级块、磁盘索引节点、文件目录区的信息写回至磁盘
{
    //用户表信息写入文件
    fstream user_info;
    user_info.open("user_information", ios::out | ios::trunc);
    if (!user_info.is_open())
    {
        cout << "ERROR: User information file open failed! System will exit.";
        getchar();
        exit(0);
    }

    for (int i = 0; i < MAXUSERNUM; i++)
    {
        //按顺序写入用户类别、用户ID、组ID、用户密码
        user_info << User[i].u_category << " " << User[i].u_uid << " " << User[i].u_gid
                  << " " << User[i].u_password << " " << User[i].u_logcount << endl;
    }

    user_info.close();

    //磁盘1区信息写入文件
    //0.打开文件
    fstream disk_info;
    disk_info.open("disk_information_part1", ios::out | ios::trunc);
    if (!disk_info.is_open())
    {
        cout << "ERROR: Disk information-part1 file open failed! System will exit.";
        getchar();
        exit(0);
    }

    //1.写入超级块的信息
    //写入i节点块数、空闲i节点块数
    disk_info << Disk.s_block.s_i_size << " " << Disk.s_block.s_free_i_size << endl;
    //写入空闲i节点数组
    for (int i = 0; i < NICINOD; i++)
        disk_info << Disk.s_block.s_free_i_array[i] << " ";
    disk_info << endl;
    //写入空闲i节点指针、铭记i节点、数据块块数、空闲数据块块数
    disk_info << Disk.s_block.s_free_i_pointer << " " << Disk.s_block.s_i_remember_node << " "
              << Disk.s_block.s_data_size << " " << Disk.s_block.s_free_data_size << endl;
    //写入空闲数据块数组
    for (int i = 0; i < NICFREE; i++)
        disk_info << Disk.s_block.s_free_data_array[i] << " ";
    disk_info << endl;
    //写入空闲数据块指针、超级块修改标志
    disk_info << Disk.s_block.s_free_data_pointer << " " << Disk.s_block.s_change_flag << endl;

    //2.写入磁盘索引节点的信息
    for (int i = 0; i < BLOCKSIZ * DINODEBLK / DINODESIZ; i++)
    {
        //按顺序写入节点ID、关联文件数、存取权限、用户ID、文件大小
        disk_info << Disk.d_i_array[i].di_file_number << " " << Disk.d_i_array[i].di_mode << " "
                  << Disk.d_i_array[i].di_uid << " " << Disk.d_i_array[i].di_type << " " << Disk.d_i_array[i].di_size << " ";
        //写入存放文件的物理块号
        for (int j = 0; j < NADDR; j++)
            disk_info << Disk.d_i_array[i].di_addr[j] << " ";
        disk_info << endl;
    }

    //3.写入i节点组长块的信息
    for (int i = 0; i < ILEADNUM; i++)
    {
        //按顺序写入每一组的组长块号、空闲块数
        disk_info << Disk.leader_i_block[i].block_number << " " << Disk.leader_i_block[i].free_count << " ";
        //写入空闲块数组
        for (int j = 0; j < IFREE; j++)
            disk_info << Disk.leader_i_block[i].free_array[j] << " ";
        disk_info << endl;
    }

    //4.写入文件组长块的信息
    for (int i = 0; i < DATALEADBLK; i++)
    {
        //按顺序写入每一组的组长块号、空闲块数
        disk_info << Disk.leader_data_block[i].block_number << " " << Disk.leader_data_block[i].free_count << " ";
        //写入空闲块数组
        for (int j = 0; j < DATAFREE; j++)
            disk_info << Disk.leader_data_block[i].free_array[j] << " ";
        disk_info << endl;
    }

    disk_info.close();

    //磁盘2区信息写入文件
    fstream disk_info_2;
    disk_info_2.open("disk_information_part2", ios::out | ios::trunc);
    if (!disk_info_2.is_open())
    {
        cout << "ERROR: Disk information-part2 file open failed! System will exit.";
        getchar();
        exit(0);
    }

    for (int i = 0; i < DATAMEMBLK; i++)
    {
        Disk.member_data_block[i][511] = '\n';
        for (int j = 0; j < BLOCKSIZ; j++)
            disk_info_2 << Disk.member_data_block[i][j];
    }

    disk_info_2.close();
}

void read_info_from_disk() //超级块、磁盘索引节点、文件目录区的信息从磁盘读入
{
    //读入用户信息
    ifstream user_info;
    user_info.open("user_information", ios::in);
    if (!user_info.is_open())
    {
        cout << "ERROR: User information file open failed! System will exit.";
        getchar();
        exit(0);
    }

    for (int i = 0; i < MAXUSERNUM; i++)
    {
        user_info >> User[i].u_category >> User[i].u_uid >> User[i].u_gid >> User[i].u_password >> User[i].u_logcount;
    }

    user_info.close();

    //读入磁盘1区信息
    //0.打开文件
    ifstream disk_info;
    disk_info.open("disk_information_part1", ios::in);
    if (!disk_info.is_open())
    {
        cout << "ERROR: Disk information-part1 file open failed! System will exit.";
        getchar();
        exit(0);
    }

    //1.读入超级块信息
    //读顺序完全等价于写顺序
    disk_info >> Disk.s_block.s_i_size >> Disk.s_block.s_free_i_size;
    for (int i = 0; i < NICINOD; i++)
    {
        disk_info >> Disk.s_block.s_free_i_array[i];
    }
    disk_info >> Disk.s_block.s_free_i_pointer >> Disk.s_block.s_i_remember_node >> Disk.s_block.s_data_size >> Disk.s_block.s_free_data_size;
    for (int i = 0; i < NICINOD; i++)
    {
        disk_info >> Disk.s_block.s_free_data_array[i];
    }
    disk_info >> Disk.s_block.s_free_data_pointer >> Disk.s_block.s_change_flag;

    //2.读入磁盘索引节点的信息
    //读顺序完全等价于写顺序
    for (int i = 0; i < BLOCKSIZ * DINODEBLK / DINODESIZ; i++)
    {
        disk_info >> Disk.d_i_array[i].di_file_number >> Disk.d_i_array[i].di_mode >> Disk.d_i_array[i].di_uid >>
            Disk.d_i_array[i].di_type >> Disk.d_i_array[i].di_size;
        for (int j = 0; j < NADDR; j++)
            disk_info >> Disk.d_i_array[i].di_addr[j];
    }

    //3.读入i节点组长块的信息
    //读顺序完全等价于写顺序
    for (int i = 0; i < ILEADNUM; i++)
    {
        disk_info >> Disk.leader_i_block[i].block_number >> Disk.leader_i_block[i].free_count;
        for (int j = 0; j < IFREE; j++)
            disk_info >> Disk.leader_i_block[i].free_array[j];
    }

    //4.读入文件组长块的信息
    //读顺序完全等价于写顺序
    for (int i = 0; i < DATALEADBLK; i++)
    {
        disk_info >> Disk.leader_data_block[i].block_number >> Disk.leader_data_block[i].free_count;
        for (int j = 0; j < DATAFREE; j++)
            disk_info >> Disk.leader_data_block[i].free_array[j];
    }

    //读入磁盘2区的信息
    ifstream disk_info_2;
    disk_info_2.open("disk_information_part2", ios::in);
    if (!disk_info_2.is_open())
    {
        cout << "ERROR: Disk information-part2 file open failed! System will exit.";
        getchar();
        exit(0);
    }

    //读顺序完全等价于写顺序
    char block_data[BLOCKSIZ];
    int index = 0;
    while (disk_info_2.getline(block_data, BLOCKSIZ))
        strcpy(Disk.member_data_block[index++], block_data);

    disk_info_2.close();
}

void disk_init() //磁盘初始化/格式化
{
    //设置内存中的信息
    open_ftable.clear();
    i_table.clear();
    user_table.clear();
    cur_user.u_basic.u_uid = 0;
    cur_dir.this_dir_id = 0;
    last_dir_list.clear();

    //初始化用户信息
    for (int i = 0; i < MAXUSERNUM / 3; i++) //第一组用户
    {
        User[i].u_category = ONLYREAD;
        User[i].u_uid = i + 1;
        User[i].u_gid = 1;
        User[i].u_password = "123456";
        User[i].u_logcount = 0;
    }

    for (int i = MAXUSERNUM / 3; i < MAXUSERNUM / 3 * 2; i++) //第二组用户
    {
        User[i].u_category = CANWRITE;
        User[i].u_uid = i + 1;
        User[i].u_gid = 2;
        User[i].u_password = "123456";
        User[i].u_logcount = 0;
    }

    for (int i = MAXUSERNUM / 3 * 2; i < MAXUSERNUM; i++) //第三组用户
    {
        User[i].u_category = ADMIN;
        User[i].u_uid = i + 1;
        User[i].u_gid = 3;
        User[i].u_password = "123456";
        User[i].u_logcount = 0;
    }

    //初始化磁盘信息
    //0.初始化磁盘上的超级块
    Disk.s_block.s_i_size = DINODEBLK;                                  //设定i节点块数
    Disk.s_block.s_free_i_size = BLOCKSIZ * DINODEBLK / DINODESIZ - 15; //设定空闲i节点块数

    for (int i = 0; i < NICINOD; i++) //设定空闲i节点数组
        Disk.s_block.s_free_i_array[i] = NICINOD - i;

    Disk.s_block.s_free_i_pointer = 34; //设定空闲i节点指针，指向下一个待分配i节点的位置
    Disk.s_block.s_i_remember_node = 2; //设定铭记i节点，！！应该！！为分配完本组空闲块后下一个到达的i节点组长块块号

    Disk.s_block.s_data_size = FILEBLK;              //设定磁盘数据块所占块数
    Disk.s_block.s_free_data_size = DATAMEMBLK - 15; //设定空闲数据块块数

    for (int i = 0; i < NICFREE; i++) //设定空闲块数组
        Disk.s_block.s_free_data_array[i] = NICFREE - i;

    Disk.s_block.s_free_data_pointer = 34; //初始化空闲数据块指针

    Disk.s_block.s_change_flag = ZEROFLAG; //初始化超级块修改标志

    //1.初始化磁盘上的 i 节点
    for (int i = 0; i < BLOCKSIZ * DINODEBLK / DINODESIZ; i++)
    {
        Disk.d_i_array[i].di_file_number = 0; //初始化关联文件数
        Disk.d_i_array[i].di_mode = 0;        //初始化存取权限
        Disk.d_i_array[i].di_uid = 0;         //初始化用户ID
        Disk.d_i_array[i].di_type = 0;        //初始化文件类型
        Disk.d_i_array[i].di_size = 0;        //初始化文件大小
        for (int j = 0; j < NADDR; j++)       //初始化物理块号
            Disk.d_i_array[i].di_addr[j] = 0;
    }

    Disk.d_i_array[0].di_file_number = 1; //初始化关联文件数
    Disk.d_i_array[0].di_mode = DISYS;    //初始化存取权限
    Disk.d_i_array[0].di_uid = FILESYS;   //初始化用户ID
    Disk.d_i_array[0].di_type = DIRFILE;  //初始化文件类型
    Disk.d_i_array[0].di_size = 67;       //初始化文件大小
    Disk.d_i_array[0].di_addr[0] = 1;

    Disk.d_i_array[1].di_file_number = 1; //初始化关联文件数
    Disk.d_i_array[1].di_mode = DISYS;    //初始化存取权限
    Disk.d_i_array[1].di_uid = FILESYS;   //初始化用户ID
    Disk.d_i_array[1].di_type = DIRFILE;  //初始化文件类型
    Disk.d_i_array[1].di_size = 43;       //初始化文件大小
    Disk.d_i_array[1].di_addr[0] = 2;

    Disk.d_i_array[2].di_file_number = 1; //初始化关联文件数
    Disk.d_i_array[2].di_mode = DISYS;    //初始化存取权限
    Disk.d_i_array[2].di_uid = FILESYS;   //初始化用户ID
    Disk.d_i_array[2].di_type = DIRFILE;  //初始化文件类型
    Disk.d_i_array[2].di_size = 23;       //初始化文件大小
    Disk.d_i_array[2].di_addr[0] = 3;

    for (int i = 3; i < 15; i++)
    {
        Disk.d_i_array[i].di_file_number = 1; //初始化关联文件数
        Disk.d_i_array[i].di_mode = DISYS;    //初始化存取权限
        Disk.d_i_array[i].di_uid = FILESYS;   //初始化用户ID
        Disk.d_i_array[i].di_type = DIRFILE;  //初始化文件类型
        Disk.d_i_array[i].di_size = 0;        //初始化文件大小
        Disk.d_i_array[i].di_addr[0] = i + 1;
    }

    //3.初始化磁盘上的i节点组长块区
    //前十个i节点组长块，每个块可以保存50个i节点
    for (int i = 0; i < ILEADNUM - 1; i++)
    {
        Disk.leader_i_block[i].block_number = i + 1; //i节点组长块编号的1~10
        Disk.leader_i_block[i].free_count = IFREE;   //初始每组空闲i节点数均为50
        for (int j = 0; j < IFREE; j++)
            Disk.leader_i_block[i].free_array[j] = IFREE - j + 50 * i; //空闲i节点数组中，由地址低→高，i节点号高→低，从高地址开始先分配低块号的块
    }

    //最后一个i节点组长块，仅保存12个i节点
    Disk.leader_i_block[ILEADNUM - 1].block_number = 11; //i节点组长块编号的12
    Disk.leader_i_block[ILEADNUM - 1].free_count = 13;   //共计512个i节点，前10个组长块共保存了500个i节点
    for (int j = 1; j <= 12; j++)
        Disk.leader_i_block[ILEADNUM - 1].free_array[j] = 12 - j + 500 + 1; //地址低→高，数块号高→低，保存0、512 ~ 501号i节点
    Disk.leader_i_block[ILEADNUM - 1].free_array[0] = 0;                    //最后一组后面没有新的组所有最后一组的第一块指向0

    //4.初始化磁盘上的文件组长块区
    for (int i = 0; i < DATALEADBLK; i++)
    {
        Disk.leader_data_block[i].block_number = 50 * i; //设定组长块编号
        Disk.leader_data_block[i].free_count = DATAFREE; //初始每组空闲块数均为50
        for (int j = 0; j < DATAFREE; j++)
            Disk.leader_data_block[i].free_array[j] = DATAFREE - j + 50 * i; //空闲块数组中，由地址低→高，数块号高→低，从高地址开始先分配低块号的块
    }
    Disk.leader_data_block[DATALEADBLK - 1].free_array[0] = 0; //每一组的第一块指示下一个组长块，最后一组后面没有新的组所有最后一组的第一块指向0

    //5.初始化磁盘上文件组员块区
    for (int i = 0; i < DATAMEMBLK; i++)
    {
        for (int j = 0; j < BLOCKSIZ; j++)
            Disk.member_data_block[i][j] = '0';
    }

    memcpy(Disk.member_data_block[0], "user 2 sys 3 bin 4 boot 5 cdrom 6 lib 7 lib64 8 media 9 temp 10 END ", 68);
    memcpy(Disk.member_data_block[1], "Zijian.Miao 11 Yang.Xu 12 Yuhang.Liu 13 END ", 44);
    memcpy(Disk.member_data_block[2], "workLog 14 tools 15 END ", 24);

    for (int i = 0; i < 3; i++)
        Disk.member_data_block[i][511] = '\n';

    for (int i = 3; i < DATAMEMBLK; i++)
    {
        memcpy(Disk.member_data_block[i], "END ", 4);
        Disk.member_data_block[i][511] = '\n';
    }

    write_info_to_disk(); //向磁盘写回信息初始化的信息
}

void open_file_system() //进入文件系统
{
    //读入磁盘信息
    read_info_from_disk();

    //初始化内存中相关信息
    //0.初始化内存i节点的哈希链表
    i_table.resize(NHINO);
    for (int i = 0; i < NHINO; i++)
    {
        i_table[i].i_pointer1 = nullptr; //指针初始指向空
        i_table[i].i_pointer2 = nullptr; //指针初始指向空
        i_table[i].i_number = IZERO;     //i节点项未具体分配i节点
        i_table[i].i_count = 0;          //引用计数为0
        i_table[i].i_mark = IEMPTY;      //初始标志
    }

    //1.初始化当前所处目录的信息
    cur_dir.this_dir_id = 0;
    change_dir(1, "root");

    //2.初始化当前操作用户的信息
    cur_user.u_basic.u_uid = 0;

    //3.初始化保存剪切/复制项信息的标志
    temp_file.mode = 0;
}

void exit_file_system() //退出文件系统
{
    //如果当前还有用户处于登录状态，无法关闭系统
    if (user_table.size() != 0)
    {
        cout << "There are still users in the current system that are logged in and cannot shut down the system!\nNot logged out user:\n";
        for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
            cout << "\tGroup: " << user_table[i].u_basic.u_gid << ", User: " << user_table[i].u_basic.u_uid << "\n";
        return;
    }
    //没有用户处于登录状态，可以关闭系统
    //将系统文件打开表的各项的i节点释放
    for (unordered_map<unsigned short, open_file_table_item>::iterator i = open_ftable.begin(); i != open_ftable.end(); i++)
        i_put(i->second.ftable_i_node);

    //用户及磁盘信息写回至文件
    write_info_to_disk();

    cout << "Thank you for using!";
    getchar();
    getchar();
    exit(0);
}

void user_login() //用户登录
{
    int gid, uid, u_index;
    string password;
    bool is_uid = false;
    bool right_pass = false;

    cout << "Input Group ID:";
    cin >> gid;
    while (gid != 1 && gid != 2 && gid != 3)
    {
        cout << "ERROR: This Group ID don't exist!\nPlease input Group ID again:";
        cin >> gid;
    }

    cout << "Input User ID:";
    while (1)
    {
        cin >> uid;
        for (int i = 0; i < MAXUSERNUM; i++)
        {
            if (User[i].u_gid == gid && User[i].u_uid == uid) //如果在该组中找到了该用户ID，则用户ID正确
            {
                is_uid = true;
                break;
            }
        }
        if (is_uid)
            break;
        else
        {
            cout << "ERROR: Group " << gid << " don't have User ID: " << uid << "!\nPlease input User ID again:";
        }
    }

    cout << "Input you password:";
    while (1)
    {
        cin >> password;
        while (password.size() > 6)
        {
            cout << "ERROR: This password is too long!\nPlease input you password again:";
            cin >> password;
        }
        for (int i = 0; i < MAXUSERNUM; i++)
        {
            if ((User[i].u_uid == uid) && (User[i].u_password == password)) //如果用户ID和口令字对应，则密码正确
            {
                right_pass = true;
                u_index = i;
                break;
            }
        }
        if (right_pass)
            break;
        else
        {
            cout << "ERROR: This password can't match you User ID!\nPlease input you Password again:";
        }
    }

    if (user_table.size() == 0)
    { //登录用户表为空，登录成功
        login_user_table_item temp;
        User[u_index].u_logcount++; //该用户登录次数加1

        temp.u_basic = User[u_index];
        user_table.push_back(temp);

        cur_user = temp; //更新当前用户

        cout << "Login successfully, welcome!" << endl;
    }
    else if (user_table.size() == USERNUM)
    { //登录用户表满，登陆失败
        cout << "ERROR: The number of logged in users reaches the upper limit! Login failed." << endl;
        return;
    }
    else
    {
        bool is_find = false;
        for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
        { //在登录用户表中查找当前登录用户的ID是否存在
            if (user_table[i].u_basic.u_uid == uid)
                is_find = true;
        }
        if (is_find)
        { //登录用户表中存在该用户，登陆失败
            cout << "ERROR: This user(User ID: " << uid << ")is already logged in! Login failed." << endl;
        }
        else if (!is_find)
        { //登录用户表中不存在该用户，登陆成功
            login_user_table_item temp;
            User[u_index].u_logcount++; //该用户登录次数加1

            temp.u_basic = User[u_index];
            user_table.push_back(temp);

            cur_user = temp; //更新当前用户

            cout << "Login successfully, welcome!" << endl;
        }
    }
}

void user_logout() //用户注销
{
    bool can_logout = false;
    string pass;

    cout << "Input you password: ";
    cin >> pass;

    if (cur_user.u_basic.u_password == pass)
        can_logout = true;

    if (can_logout)
    { //登录用户表中有准备注销用户的信息，该用户当前处于登录状态，执行注销相关的操作
        string confirm;
        cout << "Confirm logout?(y/n):";
        cin >> confirm;
        if (confirm == "y")
        {
            for (vector<login_user_table_item>::size_type i = 0; cur_user.u_ftable.size() != 0 && i < cur_user.u_ftable.size(); i++)
            {
                //如果当前注销用户的打开文件表中有项，需要该项指向的释放内存i节点，删除文件打开表中的该项
                //释放内存i节点后,然后删除系统文件打开表中的该项
                unsigned int i_number = cur_user.u_ftable[i];
                cur_user.u_ftable[i] = 0;
                i_put(open_ftable[i_number].ftable_i_node);
                open_ftable.erase(i);
            }
            for (vector<login_user_table_item>::iterator i = user_table.begin(); i != user_table.end(); i++)
            {
                if ((*i).u_basic.u_uid == cur_user.u_basic.u_uid)
                {
                    user_table.erase(i);
                    break;
                }
            }
            //设置当前用户的用户ID为0
            cur_user.u_basic.u_uid = 0;
            last_dir_list.clear();
            cout << "Logout successful!" << endl;
        }
        else
        {
            cout << "Cancel logout." << endl;
        }
    }
    else
    {
        cout << "ERROR: This password can't match you User ID, logout failed!\n";
    }
}

void user_switch() //用户切换
{
    bool is_user = false, can_switch = false;
    unsigned short uid;
    string pass;
    login_user_table_item temp_user;

    cout << "Input target switch User ID: ";
    cin >> uid;
    for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
    { //判断切换目标用户是否为当前已登录用户
        if (user_table[i].u_basic.u_uid == uid)
        {
            is_user = true;
            temp_user = user_table[i];
        }
    }
    if (is_user)
    { //验证用户密码
        cout << "Input you password: ";
        cin >> pass;
        if (temp_user.u_basic.u_password == pass)
            can_switch = true;
    }
    else
    {
        cout << "ERROR: No user with User ID: " << uid << " currently logged in, switchover failed.\n";
        return;
    }

    if (can_switch)
    { //切换用户
        cur_user = temp_user;
        cout << "Switch user successfully!\n";
    }
    else
    {
        cout << "ERROR: This password failed to match the User Id:" << uid << ", switchover failed.\n";
        return;
    }
}

void user_change_password() //用户修改密码
{
    string old_password, new_password;
    bool can_change = false, do_change = false;

    cout << "Input your old password: ";
    cin >> old_password;
    if (old_password == cur_user.u_basic.u_password)
        can_change = true;

    if (can_change)
    { //可以修改密码
        cout << "Input your new password: ";
        cin >> new_password;

        string confirm;
        cout << "Confirm logout?(y/n):";
        cin >> confirm;
        if (confirm == "y")
            do_change = true;
    }
    else
    {
        cout << "ERROR: Old password input error! Password change failed.\n";
        return;
    }

    if (do_change)
    { //执行修改密码
        //更新当前用户的信息
        cur_user.u_basic.u_password = new_password;

        //更新登录用户表的信息
        for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
        {
            if (cur_user.u_basic.u_uid == user_table[i].u_basic.u_uid)
                user_table[i].u_basic.u_password = new_password;
        }

        //更新用户信息表的信息
        for (int i = 0; i < MAXUSERNUM; i++)
        {
            if (cur_user.u_basic.u_uid == User[i].u_uid)
                User[i].u_password = new_password;
        }

        cout << "Password has been updated!\n";
    }
    else
        cout << "Change password request to cancel.\n";
}

i_node *i_alloc() //分配磁盘索引节点
{
    i_node *temp_i_node;    //临时索引结点，即返回的i结点
    unsigned int block_num; //索引i结点的编号
    if (Disk.s_block.s_free_i_pointer > 0)
    {
        block_num = Disk.s_block.s_free_i_array[Disk.s_block.s_free_i_pointer]; //找到超级块中指向的索引i节点号
        // temp_i_node->i_basic = Disk.d_i_array[block_num - 1];
        temp_i_node = i_get(block_num); //内存中有直接调用，否则从磁盘块读入内存
        Disk.s_block.s_free_i_pointer--;
        Disk.s_block.s_free_i_size--; //空闲i节点数减1
        // temp_i_node.i_flag = 1;  //标志此节点已被分配
    }
    else if (Disk.s_block.s_free_i_pointer == 0)
    {
        block_num = Disk.s_block.s_free_i_array[Disk.s_block.s_free_i_pointer]; //找到超级块中指向的索引i节点号
        if (block_num == 0)
        {
            cout << "no free_i_node for allocation" << endl;
            // temp_i_node.i_flag = 0;  //标志此节点未被分配
            return nullptr;
        }
        else
        {
            // temp_i_node->i_basic = Disk.d_i_array[block_num - 1];
            temp_i_node = i_get(block_num); //内存中有直接调用，否则从磁盘块读入内存
            // temp_i_node.i_flag = 1;  //标志此节点已被分配
            group_lead_block cur_group_lead = Disk.leader_i_block[Disk.s_block.s_i_remember_node - 1];
            Disk.s_block.s_free_i_size--; //空闲i节点数减1
            Disk.s_block.s_i_remember_node += 1;
            for (int i = 0; i < NICINOD; i++) //将组长块的信息调入超级块中
            {
                Disk.s_block.s_free_i_array[i] = cur_group_lead.free_array[i];
            }
            Disk.s_block.s_free_i_pointer = cur_group_lead.free_count - 1; //初始化空闲i节点指针
        }
    }
    return temp_i_node;
}

void i_free(unsigned int dinodeid) //释放磁盘索引节点
{
    if (Disk.s_block.s_i_remember_node == 12) //当前为最后一个组长块中的信息
    {
        if (Disk.s_block.s_free_i_pointer == 12) //12块已经全部分配，存入组长块
        {
            unsigned short cur_block_number; //当前要存入信息的组长块号
            Disk.s_block.s_i_remember_node--;
            cur_block_number = Disk.s_block.s_i_remember_node - 1;

            for (int i = 0; i < 13; i++) //信息存入组长块
            {
                Disk.leader_i_block[cur_block_number].free_array[i] = Disk.s_block.s_free_i_array[i];
            }
            Disk.s_block.s_free_i_pointer = 0; //指针指向当前空闲块
            Disk.s_block.s_free_i_array[Disk.s_block.s_free_i_pointer] = dinodeid;
            Disk.s_block.s_free_i_size++; //空闲i节点总块数加1
        }
        else if (Disk.s_block.s_free_i_pointer < 12 && Disk.s_block.s_free_i_pointer >= 0)
        {
            Disk.s_block.s_free_i_array[Disk.s_block.s_free_i_pointer + 1] = dinodeid;
            Disk.s_block.s_free_i_pointer++; //指针移向下一位
            Disk.s_block.s_free_i_size++;    //空闲i节点总块数加1
        }
    }
    else
    {
        if (Disk.s_block.s_free_i_pointer == 49)
        {
            unsigned short cur_block_number; //当前要存入信息的组长块号
            if (Disk.s_block.s_i_remember_node > 2)
            {
                Disk.s_block.s_i_remember_node--;
                cur_block_number = Disk.s_block.s_i_remember_node - 1;
            }
            else
            {
                cur_block_number = Disk.s_block.s_i_remember_node - 2;
            }

            for (int i = 0; i < NICINOD; i++) //信息存入组长块
            {
                Disk.leader_i_block[cur_block_number].free_array[i] = Disk.s_block.s_free_i_array[i];
            }
            if (cur_block_number == 0) //存入1号组长块后结束
                cout << "all free_i_node are free" << endl;
            else
            {
                Disk.s_block.s_free_i_pointer = 0;
                Disk.s_block.s_free_i_array[Disk.s_block.s_free_i_pointer] = dinodeid;
                Disk.s_block.s_free_i_size++; //空闲i节点总块数加1
            }
        }
        else if (Disk.s_block.s_free_i_pointer < 49)
        {
            Disk.s_block.s_free_i_array[Disk.s_block.s_free_i_pointer + 1] = dinodeid;
            Disk.s_block.s_free_i_pointer++; //指针指向当前空闲索引节点
            Disk.s_block.s_free_i_size++;    //空闲i节点总块数加1
        }
    }
}

i_node *i_get(unsigned int dinodeid) //获取内存索引节点
{
    int inodeid;     // 节点在哈希列表的位置
    int existed = 0; //节点在哈希表中是否存在
    i_node *temp, *newinode;
    inodeid = dinodeid % NHINO - 1; //对应128个链表中的位置
    if (inodeid == -1)
        inodeid = 127;
    if (i_table[inodeid].i_pointer1 == nullptr) //链表为空则不存在
    {
        existed = 0;
    }
    else
    {
        temp = i_table[inodeid].i_pointer1;
        while (temp)
        {
            if (temp->i_number == dinodeid) //在内存中找到了要查找的节点
            {
                existed = 1;
                // temp->i_count++; //引用次数加1
                return temp;
            }
            else
            {
                temp = temp->i_pointer1;
                existed = 0;
            }
        }
    }
    if (existed == 0)
    {
        newinode = (i_node *)malloc(sizeof(i_node));
        //将磁盘中索引节点信息复制到内存索引节点
        newinode->i_basic = Disk.d_i_array[dinodeid - 1];
        //将新节点加入到哈希表中
        newinode->i_pointer1 = i_table[inodeid].i_pointer1;
        newinode->i_pointer2 = &i_table[inodeid];
        if (newinode->i_pointer1 != nullptr)
            newinode->i_pointer1->i_pointer2 = newinode;
        i_table[inodeid].i_pointer1 = newinode;

        newinode->i_count = 1;         //引用了一次
        newinode->i_number = dinodeid; //对应为磁盘中哪一个索引节点
        newinode->i_mark = IOPEN;      //不确定   源码中将i_flag置0

        return newinode;
    }
    return nullptr;
}

void i_put(i_node *pinode) //释放内存索引节点
{
    if (pinode->i_count > 1) //还有其它文件在调用，依旧存放在内存
    {
        pinode->i_count--;
        return;
    }
    else
    {
        //文件关联数不等于0，将内存信息写回对应的磁盘
        if (pinode->i_basic.di_file_number != 0)
        {
            Disk.d_i_array[pinode->i_number - 1] = pinode->i_basic;
        }
        else //删除对应的文件信息
        {
            // block_num = pinode->i_basic.di_addr
            for (unsigned int i = 0; pinode->i_basic.di_addr[i] != 0; i++)
            {
                block_free(pinode->i_basic.di_addr[i]);
                // continue;
            }
            i_free(pinode->i_number); //释放索引节点
        }

        //释放内存索引节点
        if (pinode->i_pointer1 == nullptr)
        {
            pinode->i_pointer2->i_pointer1 = nullptr;
        }
        else
        {
            pinode->i_pointer1->i_pointer2 = pinode->i_pointer2;
            pinode->i_pointer2->i_pointer1 = pinode->i_pointer1;
        }
    }
}

unsigned short block_alloc() //分配磁盘块
{
    unsigned short free_block; //分配的数据块
    if (Disk.s_block.s_free_data_size == 0)
    { //空闲数据块为0，磁盘已满
        cout << "磁盘已满" << endl;
        return DISKFULL;
    }

    free_block = Disk.s_block.s_free_data_array[Disk.s_block.s_free_data_pointer]; //此次分配的数据块
    Disk.s_block.s_free_data_size--;                                               //减少空闲数据块
    // Disk.s_block.s_free_data_array[Disk.s_block.s_free_data_pointer] = 0;          //将分配过的超级快中的数组值变为0

    //把文件组长块中的栈（包括栈计数）读入到超级块中
    if (Disk.s_block.s_free_data_pointer == 1)
    {
        for (int i = 0; i < NICFREE; i++)
        {
            Disk.s_block.s_free_data_array[NICFREE - 1 - i] = Disk.leader_data_block[Disk.s_block.s_free_data_array[0] / 50].free_array[DATAFREE - i - 1];
        }
        Disk.s_block.s_free_data_pointer = NICFREE - 1;
    }
    else
    {
        Disk.s_block.s_free_data_pointer--; //如果指针不为1，则减一
    }
    return free_block;
}

void block_free(unsigned int block_num) //释放磁盘块
{
    //栈满，将超级块中的栈（包括栈计数）写入到文件组长块中，然后把释放的数据块放入超级块的空闲数据块数组的栈顶并置指针为1。
    if (Disk.s_block.s_free_data_pointer == NICFREE - 1)
    {
        for (int i = 0; i < NICFREE; i++)
        {
            Disk.leader_data_block[Disk.s_block.s_free_data_array[0] / 50 - 1].free_array[DATAFREE - i - 1] = Disk.s_block.s_free_data_array[NICFREE - 1 - i];
            if (i == 49)
                Disk.s_block.s_free_data_array[NICFREE - 1 - i] -= 50;
            else
                Disk.s_block.s_free_data_array[NICFREE - 1 - i] = 0;
        }
        Disk.s_block.s_free_data_pointer = 1;
    }
    //栈不满
    else
    {
        Disk.s_block.s_free_data_pointer++; //指针加一
    }
    Disk.s_block.s_free_data_size++;                                              //空闲数据块数加一
    Disk.s_block.s_free_data_array[Disk.s_block.s_free_data_pointer] = block_num; //将数据块添加到超级块的空闲数据块数组中
}

unsigned short name_i(string name) //对文件的存取搜索，返回目标文件的内存i节点编号
{
    for (vector<dir_items>::size_type i = 0; i < cur_dir.direct.size(); i++)
    {
        if (cur_dir.direct[i].dir_item_name == name)
            return cur_dir.direct[i].dir_item_ID;
    }
    return 0;
}

unsigned short i_name(string name, unsigned short ID) //在当前目录下找到一个空目录项，填写目录信息，返回该项在目录中的序号
{
    dir_items temp_i;
    temp_i.dir_item_ID = ID;
    temp_i.dir_item_name = name;
    cur_dir.direct.push_back(temp_i);
    return cur_dir.direct.size() - 1;
}

void print_cur_path() //输出当前用户所处目录路径的信息
{
    //输出当前用户
    if (cur_user.u_basic.u_uid == 0)
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        cout << "unknown user ";
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        cout << ":";
    }
    else
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        cout << "group: ";
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
        cout << cur_user.u_basic.u_gid;
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        cout << ", user: ";
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
        cout << cur_user.u_basic.u_uid;
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
    cout << " @simulated-file-system";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    cout << ":";

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE);
    for (vector<current_direct_table>::size_type i = 0; i < last_dir_list.size(); i++)
        cout << "/" << last_dir_list[i].this_dir_name;
    cout << "/" << cur_dir.this_dir_name;

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    cout << "$ ";
}

void show_dir() //显示当前目录下的文件信息
{
    if (cur_dir.direct.size() == 0)
    {
        cout << "Nothing under the current path!" << endl;
    }
    else
    {
        cout << "Current directory name: " << cur_dir.this_dir_name << ", include:\n";
        for (vector<dir_items>::size_type i = 0; i < cur_dir.direct.size(); i++)
        {
            i_node *temp_i = i_get(cur_dir.direct[i].dir_item_ID);

            cout << "\t" << setiosflags(ios::left) << setw(16) << cur_dir.direct[i].dir_item_name << "\t";
            if ((*temp_i).i_basic.di_type == DIRFILE)
                cout << "<dir>\t";
            else if ((*temp_i).i_basic.di_type == DATAFILE)
                cout << "<data>\t";

            cout << "mode: ";
            if ((*temp_i).i_basic.di_mode == DIREAD)
                cout << "ONLYREAD\t";
            else if ((*temp_i).i_basic.di_mode == DIWRITE)
                cout << "CANWRITE\t";
            else if ((*temp_i).i_basic.di_mode == DISYS)
                cout << "SYSFILE\t";

            cout << "block chain: ";
            for (int j = 0; (*temp_i).i_basic.di_addr[j] != 0; j++)
            {
                cout << (*temp_i).i_basic.di_addr[j] << " ";
            }
            cout << endl;
        }
    }
}

void make_dir(string d_name, unsigned short num) //创建新的目录
{
    if (cur_user.u_basic.u_category == ONLYREAD)
    {
        cout << "ERROR: No permission to create new directory!" << endl;
        return;
    }

    string new_name = d_name;
    while (name_i(new_name) != 0)
    {
        if (num == 1)
        {
            string choose;
            cout << "This directory already exists!\nDo you want to rename it automatically?(y/n):";
            cin >> choose;
            if (choose == "y")
            {
                new_name = d_name + "(" + to_string(num) + ")";
                num += 1;
            }
            else
            {
                cout << "Create directory cancel." << endl;
                return;
            }
        }
        else
        {
            new_name = d_name + "(" + to_string(num) + ")";
            num += 1;
        }
    }

    //获取新的i节点，创建新目录
    i_node *temp_i = i_alloc();
    (*temp_i).i_basic.di_file_number = 1;
    (*temp_i).i_basic.di_mode = DIWRITE;
    (*temp_i).i_basic.di_uid = cur_user.u_basic.u_uid;
    (*temp_i).i_basic.di_type = DIRFILE;
    (*temp_i).i_basic.di_size = 0;
    (*temp_i).i_basic.di_addr[0] = block_alloc(); //获取新的磁盘块，返回块号进行分配

    //更改当前目录项
    dir_items temp_ditem;
    temp_ditem.dir_item_name = new_name;
    temp_ditem.dir_item_ID = (*temp_i).i_number;
    cur_dir.direct.push_back(temp_ditem);

    //将新建文件夹的信息添加到系统文件打开表
    open_file_table_item temp_fitem;
    temp_fitem.ftable_count = 1;
    temp_fitem.ftable_flag = FILEOPEN;
    temp_fitem.ftable_i_node = temp_i;
    temp_fitem.ftable_offset = 0;
    open_ftable.insert(pair<unsigned short, open_file_table_item>((*temp_i).i_number, temp_fitem));

    //在系统文件打开表中查找当前目录的i节点，得到该目录文件的磁盘块号和当前写位置指针
    unsigned short b_number = open_ftable[cur_dir.this_dir_id].ftable_i_node->i_basic.di_addr[0];

    //更新后的目录项信息，写回磁盘块
    for (vector<string>::size_type i = 0; i < new_name.size(); i++)
    {
        Disk.member_data_block[b_number - 1][open_ftable[cur_dir.this_dir_id].ftable_offset] = new_name[i];
        open_ftable[cur_dir.this_dir_id].ftable_offset++;
    }
    Disk.member_data_block[b_number - 1][open_ftable[cur_dir.this_dir_id].ftable_offset++] = ' ';

    stack<char> new_id;
    for (unsigned short i = (*temp_i).i_number; i != 0; i /= 10)
        new_id.push(i % 10 + '0');

    while (!new_id.empty())
    {
        Disk.member_data_block[b_number - 1][open_ftable[cur_dir.this_dir_id].ftable_offset] = new_id.top();
        new_id.pop();
        open_ftable[cur_dir.this_dir_id].ftable_offset++;
    }

    memcpy(Disk.member_data_block[b_number - 1] + open_ftable[cur_dir.this_dir_id].ftable_offset, " END ", 5);
}

void delete_dir(string d_name) //删除目录d_name
{
    unsigned short i_number = name_i(d_name);
    if (i_number == 0)
    { //查找当前目录下是否有待删除目录
        cout << "No content item with name " << d_name << " found!" << endl;
        return;
    }

    i_node *temp_i = i_get(i_number);
    if (temp_i->i_basic.di_uid != cur_user.u_basic.u_uid && cur_user.u_basic.u_category != ADMIN)
    { //判断是否有权限删除
        cout << "ERROR: No permission to delete this file!" << endl;
        i_put(temp_i);
        return;
    }

    i_node *dir_inode = i_get(cur_dir.this_dir_id); //获得当前目录对应的i节点

    if (open_ftable.count(i_number) != 0) //判断该文件是否在系统文件打开表被打开
        open_ftable.erase(i_number);      //删除系统文件打开表汇总的内容

    //找到当前目录表中删除文件对应的项
    for (vector<dir_items>::iterator i = cur_dir.direct.begin(); i != cur_dir.direct.end(); i++)
    {
        if ((*i).dir_item_ID == i_number)
        {
            cur_dir.direct.erase(i);
            break;
        }
    }

    //初始化该文件数据块的信息
    for (int i = 0; temp_i->i_basic.di_addr[i] != 0; i++)
    {
        for (int j = 0; j < BLOCKSIZ; j++)
            Disk.member_data_block[temp_i->i_basic.di_addr[i] - 1][j] = '0';
        memcpy(Disk.member_data_block[i], "END ", 4);
        Disk.member_data_block[i][511] = '\n';
    }

    // 目录写回磁盘
    int data_block_num = open_ftable[cur_dir.this_dir_id].ftable_i_node->i_basic.di_addr[0] - 1;
    open_ftable[cur_dir.this_dir_id].ftable_offset = 0;
    string content;
    for (vector<dir_items>::size_type t = 0; t < cur_dir.direct.size(); t++)
    {
        content = cur_dir.direct[t].dir_item_name + " " + to_string(cur_dir.direct[t].dir_item_ID) + " ";
        memcpy(Disk.member_data_block[data_block_num] + open_ftable[cur_dir.this_dir_id].ftable_offset, content.c_str(), strlen(content.c_str()));
        open_ftable[cur_dir.this_dir_id].ftable_offset += strlen(content.c_str());
    }
    content = "END ";
    memcpy(Disk.member_data_block[data_block_num] + open_ftable[cur_dir.this_dir_id].ftable_offset, content.c_str(), strlen(content.c_str()));
    for (int i = open_ftable[cur_dir.this_dir_id].ftable_offset + 4; i < BLOCKSIZ; i++)
    {
        Disk.member_data_block[data_block_num][i] = '0';
    }

    if (open_ftable[cur_dir.this_dir_id].ftable_offset == 1)
        dir_inode->i_basic.di_size = 0;
    else
        dir_inode->i_basic.di_size = open_ftable[cur_dir.this_dir_id].ftable_offset + 3;

    temp_i->i_basic.di_file_number--; //i节点文件关联数减1
    i_put(temp_i);                    // 释放文件对应的i结点
}

void change_dir(unsigned short i_number, string d_name) //改变当前目录
{
    i_node *temp_i = i_get(i_number); //由i节点序号得到内存i节点的指针
    if ((*temp_i).i_basic.di_type == DATAFILE)
    {
        cout << "This item is a data file and cannot be opened as a directory!" << endl;
        return;
    }

    //更改当前目录的信息
    if (cur_dir.this_dir_id != 0)
        last_dir_list.push_back(cur_dir);
    cur_dir.this_dir_id = i_number;
    cur_dir.this_dir_name = d_name;
    cur_dir.direct.clear();

    //遍历该内存i节点指向的所有数据块
    int ftable_offset;
    for (int i = 0; (*temp_i).i_basic.di_addr[i] != 0; i++)
    {
        string item_name;         //临时保存目录名
        unsigned int item_ID = 0; //临时目录号
        bool is_name = true;      //是目录名的标志
        //遍历每个数据块的数据
        for (ftable_offset = 0; ftable_offset < BLOCKSIZ; ftable_offset++)
        {
            //如果碰到结尾标志，结束本数据块的读取
            if ((Disk.member_data_block[(*temp_i).i_basic.di_addr[i] - 1][ftable_offset] == 'E') &&
                (Disk.member_data_block[(*temp_i).i_basic.di_addr[i] - 1][ftable_offset + 1] == 'N') &&
                (Disk.member_data_block[(*temp_i).i_basic.di_addr[i] - 1][ftable_offset + 2] == 'D'))
                break;

            if (Disk.member_data_block[(*temp_i).i_basic.di_addr[i] - 1][ftable_offset] != ' ')
            { //如果读到的字符不为空格，累积当前读过的字符
                if (is_name)
                { //当前读入的是目录名
                    item_name += Disk.member_data_block[(*temp_i).i_basic.di_addr[i] - 1][ftable_offset];
                }
                else if (!is_name)
                { //当前读入的是目录号
                    item_ID = item_ID * 10 + (Disk.member_data_block[(*temp_i).i_basic.di_addr[i] - 1][ftable_offset] - '0');
                }
            }
            else
            { //如果碰到空格
                if (is_name)
                { //当前读完的是目录名
                    is_name = false;
                }
                else if (!is_name)
                { //当前读完的是目录号
                    dir_items temp_item;
                    temp_item.dir_item_name = item_name;
                    temp_item.dir_item_ID = item_ID;
                    cur_dir.direct.push_back(temp_item);
                    item_name.clear();
                    item_ID = 0;
                    is_name = true;
                }
            }
        }
    }

    //如果新打开的目录没有在系统文件表里，更新系统文件打开表
    if (open_ftable.count(i_number) == 0)
    {
        open_file_table_item temp;
        temp.ftable_i_node = temp_i;
        temp.ftable_offset = ftable_offset;
        temp.ftable_count = 1;
        temp.ftable_flag = FILEOPEN;
        open_ftable.insert(pair<unsigned short, open_file_table_item>(i_number, temp));
    }
}

bool access(i_node *inode, unsigned short mode) //判定用户对文件是否拥有某种操作权限
{
    switch (mode)
    {
    case READ: //读文件的权限判断
        if ((inode->i_basic.di_mode == DIREAD) || (inode->i_basic.di_mode == DIWRITE))
            return true; //文件可写或者可读，任何用户都可以读
        if ((inode->i_basic.di_mode == DISYS) && (cur_user.u_basic.u_category == ADMIN))
            return true; //系统数据文件，只有管理员可以读
        return false;
    case WRITE: //写文件权限判断
        if ((inode->i_basic.di_mode == DIWRITE) && ((cur_user.u_basic.u_category == ONLYREAD) || (cur_user.u_basic.u_category == ADMIN)))
            return true; //可写文件并且是可写用户
        if ((inode->i_basic.di_mode == DISYS) && (cur_user.u_basic.u_category == ADMIN))
            return true; //管理员写系统文件
        return false;
    default:
        return false;
    }
}

void open_file(string filename) //打开文件,返回用户文件打开表中的位置
{
    unsigned int dinodeid;

    dinodeid = name_i(filename);
    if (dinodeid == 0)
    { //查询文件是否存在
        cout << "ERROR: This file does not exist!\n";
        return;
    }

    i_node *inode = i_get(dinodeid);
    //判断打开的是否为目录文件
    if (inode->i_basic.di_type == DIRFILE)
    {
        cout << "ERROR: Cannot open the catalog file in this form, please use the cd command!" << endl;
        i_put(inode);
        return;
    }

    // 查询是否有权限打开文件
    if (!access(inode, READ))
    {
        cout << "ERROR: No permission to open file!" << endl;
        i_put(inode);
        return;
    }

    //判断系统中是否可以继续打开文件
    if (open_ftable.size() == SYSOPENFILE)
    {
        cout << "ERROR: The number of open files in the current system has reached the upper limit!" << endl;
        i_put(inode);
        return;
    }

    //判断用户是否可以继续打开文件
    if (cur_user.u_ftable.size() == NOFILE) // 判断用户文件打开表是否已满
    {
        cout << "ERROR: The number of open files in the user: " << cur_user.u_basic.u_uid << " has reached the upper limit!" << endl;
        i_put(inode);
        return;
    }

    //判断该文件是否已经打开
    if (open_ftable.count(inode->i_number) != 0)
    {
        cout << "ERROR: This file has been opened in the system!" << endl;
        i_put(inode);
        return;
    }

    //添加文件信息到系统文件打开表
    open_file_table_item temp;
    temp.ftable_count = 1;
    temp.ftable_flag = FILEOPEN;
    temp.ftable_i_node = inode;
    temp.ftable_offset = 0;
    open_ftable.insert(pair<unsigned short, open_file_table_item>(inode->i_number, temp));

    //添加文件信息到用户文件打开表
    for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
    {
        if (user_table[i].u_basic.u_uid == cur_user.u_basic.u_uid)
        {
            user_table[i].u_ftable.push_back(inode->i_number);
            cur_user = user_table[i];
            break;
        }
    }
}

void close_file(string filename) //关闭文件
{
    unsigned short i_number = name_i(filename);
    bool can_close = false;
    vector<unsigned short>::iterator i;

    //查询当前目录下文件是否存在
    if (i_number == 0)
    {
        cout << "ERROR: This file does not exist!\n";
        return;
    }

    for (i = cur_user.u_ftable.begin(); i != cur_user.u_ftable.end(); i++)
    { //如果用户文件打开表中有该项，可以关闭文件
        if ((*i) == i_number)
        {
            can_close = true;
            break;
        }
    }

    if (can_close)
    {
        i_node *inode = open_ftable[i_number].ftable_i_node;
        i_put(inode);
        open_ftable.erase(i_number);
        cur_user.u_ftable.erase(i);
        return;
    }
    else
    {
        for (unordered_map<unsigned short, open_file_table_item>::iterator i = open_ftable.begin(); i != open_ftable.end(); i++)
        {
            if (i->first == i_number)
            { //如果在系统文件打开表找到了该项，则是其他用户打开的文件
                cout << "ERROR: You cannot close files opened by other users!\n";
                return;
            }
        }
        //如果没有找到，则是该文件未打开
        cout << "ERROR: The file is not open!Cannot be closed.\n";
        return;
    }
}

void create_file(string filename, unsigned short mode, unsigned short num) //创建文件,返回用户文件打开表中的位置
{
    i_node *inode, *dir_inode;
    unsigned short i;
    string new_name = filename;

    while (name_i(new_name) != 0)
    {
        if (num == 1)
        {
            string choose;
            cout << "This file already exists!\nDo you want to rename it automatically?(y/n):";
            cin >> choose;
            if (choose == "y")
            {
                new_name = filename + "(" + to_string(num) + ")";
                num += 1;
            }
            else
            {
                cout << "Create directory cancel." << endl;
                return;
            }
        }
        else
        {
            new_name = filename + "(" + to_string(num) + ")";
            num += 1;
        }
    }

    // 查询是否有权限创建文件
    if (cur_user.u_basic.u_category == ONLYREAD)
    {
        cout << "ERROR: No permission to create file!" << endl;
        return;
    }

    //判断系统中是否可以继续添加文件
    if (open_ftable.size() == SYSOPENFILE)
    {
        cout << "ERROR: The number of open files in the current system has reached the upper limit!" << endl;
        return;
    }

    //判断用户是否可以继续添加文件
    if (cur_user.u_ftable.size() == NOFILE) // 判断用户文件打开表是否已满
    {
        cout << "ERROR: The number of open files in the user: " << cur_user.u_basic.u_uid << " has reached the upper limit!" << endl;
        return;
    }

    inode = i_alloc();
    i_name(new_name, inode->i_number);
    dir_inode = i_get(cur_dir.this_dir_id);

    //将新加文件的目录信息写回磁盘
    string content = new_name + " " + to_string(inode->i_number) + " ";
    memcpy(Disk.member_data_block[(*dir_inode).i_basic.di_addr[0] - 1] + open_ftable[cur_dir.this_dir_id].ftable_offset,
           content.c_str(), strlen(content.c_str()));
    dir_inode->i_basic.di_size += strlen(content.c_str());
    open_ftable[cur_dir.this_dir_id].ftable_offset += strlen(content.c_str());

    memcpy(Disk.member_data_block[(*dir_inode).i_basic.di_addr[0] - 1] + open_ftable[cur_dir.this_dir_id].ftable_offset, "END ", 4);
    dir_inode->i_basic.di_size += 3;

    inode->i_basic.di_mode = mode; //文件初始化为用户类别权限（是否给初始权限）
    inode->i_basic.di_uid = cur_user.u_basic.u_uid;
    inode->i_basic.di_type = DATAFILE;
    inode->i_basic.di_size = 0;                //初始化文件大小为0，此时文件中无数据
    inode->i_basic.di_addr[0] = block_alloc(); //初始分配一个物理块
    inode->i_basic.di_file_number = 1;         //关联文件数为1

    //若当前数组没有则添加到数组尾
    open_file_table_item cur_creat_file;
    cur_creat_file.ftable_count = 1;
    cur_creat_file.ftable_flag = FILEOPEN;
    cur_creat_file.ftable_i_node = inode;
    cur_creat_file.ftable_offset = 0;
    open_ftable.insert(pair<unsigned short, open_file_table_item>((*inode).i_number, cur_creat_file));

    //添加到用户打开表
    for (i = 0; i < user_table.size(); i++)
    {
        if (user_table[i].u_basic.u_uid == cur_user.u_basic.u_uid)
        {
            user_table[i].u_ftable.push_back((*inode).i_number);
            cur_user = user_table[i];
            break;
        }
    }
}

void delete_file(string filename) //删除文件
{
    unsigned short dinodeid;
    i_node *inode, *dir_inode;
    dinodeid = name_i(filename);
    if (dinodeid != 0) //找到文件对应的内存i结点
    {
        inode = i_get(dinodeid);
        if (inode->i_basic.di_uid != cur_user.u_basic.u_uid && cur_user.u_basic.u_category != ADMIN)
        {
            cout << "ERROR: No permission to delete this file!" << endl;
            i_put(inode);
            return;
        }
    }
    else
    {
        cout << "No content item with name " << filename << " found!\n";
        return;
    }

    dir_inode = i_get(cur_dir.this_dir_id); //获得当前目录对应的i节点
    inode->i_basic.di_file_number--;        //i节点文件关联数减1

    if (open_ftable.count(dinodeid) != 0) //判断该文件是否在系统文件打开表被打开
        open_ftable.erase(dinodeid);      //删除系统文件打开表汇总的内容

    //如果用户打开了，在用户打开文件表中删除
    for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
    {
        if (user_table[i].u_basic.u_uid == cur_user.u_basic.u_uid)
        {
            for (vector<unsigned short>::iterator iter = user_table[i].u_ftable.begin(); iter != user_table[i].u_ftable.end(); iter++)
            {
                if ((*iter) == dinodeid)
                    user_table[i].u_ftable.erase(iter);
                cur_user = user_table[i];
                break;
            }
        }
    }

    //在当前目录表中删除该项
    for (vector<dir_items>::iterator iter = cur_dir.direct.begin(); iter != cur_dir.direct.end(); iter++)
    {
        if ((*iter).dir_item_name == filename)
        {
            cur_dir.direct.erase(iter);
            break;
        }
    }
    //目录写回磁盘
    int data_block_num = open_ftable[cur_dir.this_dir_id].ftable_i_node->i_basic.di_addr[0] - 1;
    open_ftable[cur_dir.this_dir_id].ftable_offset = 0;
    string content;
    for (vector<dir_items>::size_type t = 0; t < cur_dir.direct.size(); t++)
    {
        content = cur_dir.direct[t].dir_item_name + " " + to_string(cur_dir.direct[t].dir_item_ID) + " ";
        memcpy(Disk.member_data_block[data_block_num] + open_ftable[cur_dir.this_dir_id].ftable_offset, content.c_str(), strlen(content.c_str()));
        open_ftable[cur_dir.this_dir_id].ftable_offset += strlen(content.c_str());
    }
    content = "END ";
    memcpy(Disk.member_data_block[data_block_num] + open_ftable[cur_dir.this_dir_id].ftable_offset, content.c_str(), strlen(content.c_str()));
    for (int i = open_ftable[cur_dir.this_dir_id].ftable_offset + 4; i < BLOCKSIZ; i++)
    {
        Disk.member_data_block[data_block_num][i] = '0';
    }

    if (open_ftable[cur_dir.this_dir_id].ftable_offset == 1)
        dir_inode->i_basic.di_size = 0;
    else
        dir_inode->i_basic.di_size = open_ftable[cur_dir.this_dir_id].ftable_offset + 3;

    //删除后的数据块信息写回磁盘
    for (int i = 0; inode->i_basic.di_addr[i] != 0; i++)
    {
        for (int j = 0; j < BLOCKSIZ; j++)
            Disk.member_data_block[inode->i_basic.di_addr[i] - 1][j] = '0';
        memcpy(Disk.member_data_block[inode->i_basic.di_addr[i] - 1], "END ", 4);
        Disk.member_data_block[i][511] = '\n';
    }

    // 释放文件对应的i结点
    i_put(inode);
}

void cut_file(string f_name) //剪切文件
{
    //判断当前目录下是否存在文件
    unsigned short i_number = name_i(f_name);
    if (i_number == 0)
    {
        cout << "ERROR: This file does not exist!\n";
        return;
    }

    //判断是否有权限
    i_node *temp_i = i_get(i_number);
    if (cur_user.u_basic.u_category == ONLYREAD)
    {
        cout << "ERROR: No permission to cut this file!\n";
        i_put(temp_i);
        return;
    }

    if (temp_i->i_basic.di_type == DIRFILE)
    {
        cout << "ERROR: You cannot move the entire folder!\n";
        i_put(temp_i);
        return;
    }

    //保存文件信息
    temp_file.mode = 1;
    temp_file.f_name = f_name;
    temp_file.f_i_basic = temp_i->i_basic;
    temp_file.old_dir = cur_dir;
    temp_file.f_data.clear();
    for (int i = 0; temp_i->i_basic.di_addr[i] != 0; i++)
        temp_file.f_data.push_back(Disk.member_data_block[temp_i->i_basic.di_addr[i] - 1]);
}

void copy_file(string f_name) //复制文件
{
    //判断当前目录下是否存在文件
    unsigned short i_number = name_i(f_name);
    if (i_number == 0)
    {
        cout << "ERROR: This file does not exist!\n";
        return;
    }

    //判断是否有权限
    i_node *temp_i = i_get(i_number);
    if (cur_user.u_basic.u_category == ONLYREAD)
    {
        cout << "ERROR: No permission to cut this file!\n";
        i_put(temp_i);
        return;
    }

    if (temp_i->i_basic.di_type == DIRFILE)
    {
        cout << "ERROR: You cannot move the entire folder!\n";
        i_put(temp_i);
        return;
    }

    //保存文件信息
    temp_file.mode = 2;
    temp_file.f_name = f_name;
    temp_file.f_i_basic = temp_i->i_basic;
    temp_file.f_data.clear();
    for (int i = 0; temp_i->i_basic.di_addr[i] != 0; i++)
        temp_file.f_data.push_back(Disk.member_data_block[temp_i->i_basic.di_addr[i] - 1]);
}

void paste_file() //粘贴文件
{
    if (temp_file.mode == 0)
    {
        cout << "ERROR: There is currently no content to paste!\n";
        return;
    }

    i_node *inode, *dir_inode;
    unsigned short num = 1;
    string new_name = temp_file.f_name;

    while (name_i(new_name) != 0)
    {
        if (num == 1)
        {
            string choose;
            cout << "This file already exists!\nDo you want to rename it automatically?(y/n):";
            cin >> choose;
            if (choose == "y")
            {
                new_name = temp_file.f_name + "(" + to_string(num) + ")";
                num += 1;
            }
            else
            {
                cout << "Create directory cancel." << endl;
                return;
            }
        }
        else
        {
            new_name = temp_file.f_name + "(" + to_string(num) + ")";
            num += 1;
        }
    }

    // 查询是否有权限创建文件
    if (cur_user.u_basic.u_category == ONLYREAD)
    {
        cout << "ERROR: No permission to paste file!" << endl;
        return;
    }

    //判断系统中是否可以继续添加文件
    if (open_ftable.size() == SYSOPENFILE)
    {
        cout << "ERROR: The number of open files in the current system has reached the upper limit!" << endl;
        return;
    }

    //判断用户是否可以继续添加文件
    if (cur_user.u_ftable.size() == NOFILE) // 判断用户文件打开表是否已满
    {
        cout << "ERROR: The number of open files in the user: " << cur_user.u_basic.u_uid << " has reached the upper limit!" << endl;
        return;
    }

    inode = i_alloc();
    i_name(new_name, inode->i_number);
    dir_inode = i_get(cur_dir.this_dir_id);

    //将新加文件的目录信息写回磁盘
    string content = new_name + " " + to_string(inode->i_number) + " ";
    memcpy(Disk.member_data_block[(*dir_inode).i_basic.di_addr[0] - 1] + open_ftable[cur_dir.this_dir_id].ftable_offset,
           content.c_str(), strlen(content.c_str()));
    dir_inode->i_basic.di_size += strlen(content.c_str());
    open_ftable[cur_dir.this_dir_id].ftable_offset += strlen(content.c_str());

    memcpy(Disk.member_data_block[(*dir_inode).i_basic.di_addr[0] - 1] + open_ftable[cur_dir.this_dir_id].ftable_offset, "END ", 4);
    dir_inode->i_basic.di_size += 3;

    inode->i_basic.di_file_number = temp_file.f_i_basic.di_file_number;
    inode->i_basic.di_mode = temp_file.f_i_basic.di_mode;
    inode->i_basic.di_size = temp_file.f_i_basic.di_size;
    inode->i_basic.di_type = temp_file.f_i_basic.di_type;
    inode->i_basic.di_uid = temp_file.f_i_basic.di_uid;
    for (vector<char *>::size_type i = 0; i < temp_file.f_data.size(); i++)
    {
        inode->i_basic.di_addr[i] = block_alloc(); //分配一个物理块
        strcpy(Disk.member_data_block[inode->i_basic.di_addr[i] - 1], temp_file.f_data[i]);
    }

    //若当前数组没有则添加到数组尾
    open_file_table_item cur_creat_file;
    cur_creat_file.ftable_count = 1;
    cur_creat_file.ftable_flag = FILEOPEN;
    cur_creat_file.ftable_i_node = inode;
    cur_creat_file.ftable_offset = 0;
    open_ftable.insert(pair<unsigned short, open_file_table_item>((*inode).i_number, cur_creat_file));

    //添加到用户打开表
    for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
    {
        if (user_table[i].u_basic.u_uid == cur_user.u_basic.u_uid)
        {
            user_table[i].u_ftable.push_back((*inode).i_number);
            cur_user = user_table[i];
            break;
        }
    }

    //如果为剪切过来的文件，删除前面的文件
    if (temp_file.mode == 1)
    {
        current_direct_table old_dir = cur_dir; //保存当前目录

        for (vector<current_direct_table>::size_type i = 0; i < last_dir_list.size(); i++) //删除当前目录前级列表中的内容
        {
            if (last_dir_list[i].this_dir_id == temp_file.old_dir.this_dir_id)
            {
                for (vector<dir_items>::iterator iter = last_dir_list[i].direct.begin(); iter != last_dir_list[i].direct.end(); iter++)
                {
                    if (iter->dir_item_name == temp_file.f_name)
                    {
                        last_dir_list[i].direct.erase(iter);
                        break;
                    }
                }
            }
        }
        cur_dir = temp_file.old_dir;   //打开旧目录
        delete_file(temp_file.f_name); //删除文件
        cur_dir = old_dir;             //切换回当前目录
        temp_file.mode = 0;
    }
}

void read_file(string f_name) //读文件
{
    unsigned short i_number = name_i(f_name);
    if (i_number == 0)
    {
        cout << "ERROR: This file does not exist!\n";
        return;
    }

    if (open_ftable.count(i_number) == 0)
    {
        cout << "ERROR: Please open the file before reading!\n";
        return;
    }

    //读取结果信息
    string read_result;
    i_node *temp_i = open_ftable[i_number].ftable_i_node;
    for (int i = 0; temp_i->i_basic.di_addr[i] != 0; i++)
    {
        unsigned short b_number = temp_i->i_basic.di_addr[i] - 1;
        open_ftable[i_number].ftable_offset = 0;
        while (1)
        {
            //检测文件结束标志
            if (Disk.member_data_block[b_number][open_ftable[i_number].ftable_offset] == 'E' &&
                Disk.member_data_block[b_number][open_ftable[i_number].ftable_offset + 1] == 'N' &&
                Disk.member_data_block[b_number][open_ftable[i_number].ftable_offset + 2] == 'D')
                break;
            //保存结果到数组
            read_result.push_back(Disk.member_data_block[b_number][open_ftable[i_number].ftable_offset]);
            open_ftable[i_number].ftable_offset++;
        }
    }

    //输出结果信息
    if (read_result.size() == 0)
    {
        cout << "WARNING: This file is empty and has no content!\n";
    }
    else
    {
        for (basic_string<char>::size_type i = 0; i < read_result.size(); i++)
            cout << read_result[i];
        cout << endl;
    }
}

void write_file(string filename, string type) //写文件
{
    i_node *inode;
    unsigned short dinodeid;
    vector<unsigned short>::iterator i;
    string cmd, content;
    dinodeid = name_i(filename);

    if (cur_user.u_basic.u_category == ONLYREAD)
    {
        cout << "ERROR: No permission to write file!\n";
        return;
    }

    if (dinodeid == 0)
    { //文件不存在
        cout << "ERROR: This file does not exist!\n";
        cout << "Do you want to create and open a file and write it	(y/n)?\n";
        cin >> cmd;
        if (cmd == "y")
        {
            create_file(filename, 2, 1);
            // open_file(filename);
        }
        else if (cmd == "n")
            return;
        else
        {
            cout << "ERROR: Unrecognized character";
            return;
        }
    }

    inode = i_get(dinodeid);
    if (!access(inode, WRITE))
    {
        i_put(inode);
        cout << "ERROR: This file not allowed to write\n";
        return;
    }

    if (open_ftable.count(dinodeid) == 0)
    {
        cout << "ERROR: Please open the file before writing!\n";
        i_put(inode);
        return;
    }

    // 更改系统文件打开表信息
    open_ftable[dinodeid].ftable_flag = FILECHANGE;

    for (i = cur_user.u_ftable.begin(); i != cur_user.u_ftable.end(); i++)
    { //在用户文件打开表中有找该项
        if ((*i) == dinodeid)
        {
            break;
        }
    }
    if (i == cur_user.u_ftable.end())
        cur_user.u_ftable.push_back(dinodeid);

    cout << "please input the content you want to save:\n";
    getline(cin, content);
    getline(cin, content);
    //content = "123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789";
    unsigned short b_number;
    if (type == "1") //从头写
    {
        open_ftable[dinodeid].ftable_offset = 0;
        basic_string<char>::size_type index = 0;
        for (basic_string<char>::size_type i = 0; index < content.size(); open_ftable[dinodeid].ftable_offset = 0, i++)
        {

            if (inode->i_basic.di_addr[i] != 0)
            {
                b_number = inode->i_basic.di_addr[i] - 1;
            }
            else
            {
                inode->i_basic.di_addr[i] = block_alloc();
                b_number = inode->i_basic.di_addr[i] - 1;
            }

            for (; index < content.size(); index++)
            {
                if (open_ftable[dinodeid].ftable_offset >= 507)
                {
                    Disk.member_data_block[b_number][507] = ' ';
                    Disk.member_data_block[b_number][508] = 'E';
                    Disk.member_data_block[b_number][509] = 'N';
                    Disk.member_data_block[b_number][510] = 'D';
                    Disk.member_data_block[b_number][511] = '\n';
                    break;
                }

                Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset] = content[index];
                open_ftable[dinodeid].ftable_offset++;
            }
        }
        Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset] = ' ';
        Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset + 1] = 'E';
        Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset + 2] = 'N';
        Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset + 3] = 'D';
    }
    else if (type == "0") //追加写
    {
        int i;
        //找到最后一个存放数据的物理块
        for (i = 0; inode->i_basic.di_addr[i] != 0;)
        {
            if (inode->i_basic.di_addr[i + 1] == 0)
                break;
            else
                i++;
        }
        //找到最后一个存放数据的物理块的写位置
        open_ftable[dinodeid].ftable_offset = 0;
        while (1)
        {
            if ((Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset] == 'E') &&
                (Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset] == 'N') &&
                (Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset] == 'D'))
                break;
        }
        basic_string<char>::size_type index = 0;
        for (; index < content.size(); open_ftable[dinodeid].ftable_offset = 0, i++)
        {
            unsigned short b_number;
            if (inode->i_basic.di_addr[i] != 0)
            {
                b_number = inode->i_basic.di_addr[i] - 1;
            }
            else
            {
                inode->i_basic.di_addr[i] = block_alloc();
                b_number = inode->i_basic.di_addr[i] - 1;
            }

            for (; index < content.size(); index++)
            {
                if (open_ftable[dinodeid].ftable_offset >= 507)
                {
                    Disk.member_data_block[b_number][507] = ' ';
                    Disk.member_data_block[b_number][508] = 'E';
                    Disk.member_data_block[b_number][509] = 'N';
                    Disk.member_data_block[b_number][510] = 'D';
                    Disk.member_data_block[b_number][511] = '\n';
                    break;
                }
                Disk.member_data_block[b_number][open_ftable[dinodeid].ftable_offset] = content[index];
                open_ftable[dinodeid].ftable_offset++;
            }
        }
    }

    // content = content + " END ";
    // block_num = open_ftable[dinodeid].ftable_i_node->i_basic.di_size / BLOCKSIZ;          //现在文件存了几块
    // int data_block = open_ftable[dinodeid].ftable_i_node->i_basic.di_addr[block_num] - 1; //写的物理块号
    // if (type == "0")                                                                      //从文件尾开始写
    // {
    //     if (open_ftable[dinodeid].ftable_i_node->i_basic.di_size == 0)
    //     {
    //         write_offset = 0;
    //         open_ftable[dinodeid].ftable_i_node->i_basic.di_size = 4;
    //     }
    //     else
    //         write_offset = open_ftable[dinodeid].ftable_offset - 4;
    //     // inode->i_basic.di_size = 0;
    // }
    // else if (type == "1") //从文件头开始写，覆盖原文件
    // {
    //     write_offset = 0;
    //     inode->i_basic.di_size = 0;
    //     data_block = open_ftable[dinodeid].ftable_i_node->i_basic.di_addr[0] - 1;
    //     for (int j = 1; j <= block_num; j++)
    //     {
    //         block_free(inode->i_basic.di_addr[j]);
    //     }
    // }
    // if (write_offset + strlen(content.c_str()) < BLOCKSIZ)
    // {
    //     memcpy(Disk.member_data_block[data_block] + write_offset, content.c_str(), strlen(content.c_str()));
    // }
    // else
    // {
    //     memcpy(Disk.member_data_block[data_block] + write_offset, content.c_str(), BLOCKSIZ - write_offset);
    //     for (int t = 0; t < (write_offset + strlen(content.c_str())) / BLOCKSIZ; t++)
    //     {
    //         unsigned short new_block = block_alloc();
    //         inode->i_basic.di_addr[block_num + t + 1] = new_block;
    //         write_offset = 0;
    //         memcpy(Disk.member_data_block[data_block + 1] + write_offset, content.c_str() + BLOCKSIZ - write_offset, strlen(content.c_str()) - BLOCKSIZ + write_offset);
    //     }
    //     // unsigned short new_block = block_alloc();
    //     // inode->i_basic.di_addr[block_num + 1] = new_block;
    //     // write_offset = 0;
    //     // memcpy(Disk.member_data_block[data_block + 1] + write_offset, content.c_str() + BLOCKSIZ - write_offset, strlen(content.c_str()) - BLOCKSIZ + write_offset);
    // }

    // open_ftable[dinodeid].ftable_i_node->i_basic.di_size += strlen(content.c_str());
    // if (type == "0")
    //     open_ftable[dinodeid].ftable_i_node->i_basic.di_size -= 4;
    // open_ftable[dinodeid].ftable_offset = inode->i_basic.di_size % BLOCKSIZ;

    // // return 0;
}

void search_file_or_dir(string name)
{
    //设定保存搜索结果的容器
    vector<i_node *> result_i_nodes;
    vector<vector<current_direct_table>> result_pathes;

    //保存旧目录信息
    current_direct_table old_dir = cur_dir;
    vector<current_direct_table> old_last_dir_list = last_dir_list;

    //返回根目录，开始搜索
    cur_dir.this_dir_id = 0;
    last_dir_list.clear();
    change_dir(1, "root");
    search_file_or_dir_core(name, &result_i_nodes, &result_pathes);

    //输出搜索的结果
    if (result_i_nodes.size() == 0)
    {
        cout << "ERROR: No file found named: " << name << "!";
        return;
    }
    cout << "Target name: " << name << ", " << result_i_nodes.size() << " search results found:\n";
    for (vector<i_node *>::size_type i = 0; i < result_i_nodes.size() && i < result_pathes.size(); i++)
    {

        if (result_i_nodes[i]->i_basic.di_type == DIRFILE)
            cout << "\t<dir>\t";
        else if (result_i_nodes[i]->i_basic.di_type == DATAFILE)
            cout << "\t<data>\t";
        else if (result_i_nodes[i]->i_basic.di_type == DISYS)
            cout << "\t<data>\t";
        for (vector<vector<current_direct_table>>::size_type j = 0; j < result_pathes[i].size(); j++)
            cout << "/" << result_pathes[i][j].this_dir_name;
        cout << "/" << name << endl;
    }

    //恢复之前的目录
    cur_dir = old_dir;
    last_dir_list = old_last_dir_list;
}

void search_file_or_dir_core(string name, vector<i_node *> *result_i, vector<vector<current_direct_table>> *result_path)
{
    for (vector<dir_items>::size_type i = 0; i < cur_dir.direct.size(); i++)
    {
        //找到相同名字的内容，加入至结果信息容器
        if (name == cur_dir.direct[i].dir_item_name)
        {
            (*result_i).push_back(i_get(cur_dir.direct[i].dir_item_ID));
            last_dir_list.push_back(cur_dir);
            (*result_path).push_back(last_dir_list);
            last_dir_list.pop_back();
        }

        //如果找到的是数据文件，搜索下一个
        if (i_get(cur_dir.direct[i].dir_item_ID)->i_basic.di_type == DATAFILE)
            continue;

        //如果找到的是目录文件，递归搜索
        if (i_get(cur_dir.direct[i].dir_item_ID)->i_basic.di_type == DIRFILE)
        {
            change_dir(cur_dir.direct[i].dir_item_ID, cur_dir.direct[i].dir_item_name);
            search_file_or_dir_core(name, result_i, result_path);
            cur_dir = last_dir_list[last_dir_list.size() - 1];
            last_dir_list.pop_back();
        }
    }
}

void rename(string old_name, string new_name)
{
    //在当前目录表中查找该项
    unsigned short i_number = name_i(old_name);
    if (i_number == 0)
    {
        cout << "ERROR: This file does not exist!\n";
        return;
    }

    //在当前目录表中删除该项
    for (vector<dir_items>::size_type i = 0; i < cur_dir.direct.size(); i++)
    {
        if (cur_dir.direct[i].dir_item_name == old_name)
        {
            cur_dir.direct[i].dir_item_name = new_name;
            break;
        }
    }

    //新的当前目录写回磁盘
    i_node *dir_inode = i_get(cur_dir.this_dir_id); //获得当前目录对应的i节点
    int data_block_num = open_ftable[cur_dir.this_dir_id].ftable_i_node->i_basic.di_addr[0] - 1;
    open_ftable[cur_dir.this_dir_id].ftable_offset = 0;
    string content;
    for (vector<dir_items>::size_type t = 0; t < cur_dir.direct.size(); t++)
    {
        content = cur_dir.direct[t].dir_item_name + " " + to_string(cur_dir.direct[t].dir_item_ID) + " ";
        memcpy(Disk.member_data_block[data_block_num] + open_ftable[cur_dir.this_dir_id].ftable_offset, content.c_str(), strlen(content.c_str()));
        open_ftable[cur_dir.this_dir_id].ftable_offset += strlen(content.c_str());
    }
    content = "END ";
    memcpy(Disk.member_data_block[data_block_num] + open_ftable[cur_dir.this_dir_id].ftable_offset, content.c_str(), strlen(content.c_str()));
    if (open_ftable[cur_dir.this_dir_id].ftable_offset == 1)
        dir_inode->i_basic.di_size = 0;
    else
        dir_inode->i_basic.di_size = open_ftable[cur_dir.this_dir_id].ftable_offset + 3;
}

void test_print() //测试输出
{
    for (int i = 0; i < MAXUSERNUM; i++)
    {
        cout << User[i].u_category << " " << User[i].u_uid << " " << User[i].u_gid << " "
             << User[i].u_password << " " << User[i].u_logcount << endl;
    }

    cout << Disk.s_block.s_i_size << " " << Disk.s_block.s_free_i_size << endl;
    for (int i = 0; i < NICINOD; i++)
    {
        cout << Disk.s_block.s_free_i_array[i] << " ";
    }
    cout << endl;
    cout << Disk.s_block.s_free_i_pointer << " " << Disk.s_block.s_i_remember_node << " "
         << Disk.s_block.s_data_size << " " << Disk.s_block.s_free_data_size << endl;
    for (int i = 0; i < NICFREE; i++)
    {
        cout << Disk.s_block.s_free_data_array[i] << " ";
    }
    cout << endl;
    cout << Disk.s_block.s_free_data_pointer << " " << Disk.s_block.s_change_flag << endl;

    for (int i = 0; i < 10; i++)
    {
        cout << Disk.d_i_array[i].di_file_number << " " << Disk.d_i_array[i].di_mode << " " << Disk.d_i_array[i].di_uid
             << " " << Disk.d_i_array[i].di_type << " " << Disk.d_i_array[i].di_size << " ";
        for (int j = 0; j < NADDR; j++)
            cout << Disk.d_i_array[i].di_addr[j] << " ";
        cout << endl;
    }

    for (int i = 0; i < ILEADNUM; i++)
    {
        cout << Disk.leader_i_block[i].block_number << " " << Disk.leader_i_block[i].free_count << " ";
        for (int j = 0; j < IFREE; j++)
            cout << Disk.leader_i_block[i].free_array[j] << " ";
        cout << endl;
    }

    for (int i = 0; i < DATALEADBLK; i++)
    {
        cout << Disk.leader_data_block[i].block_number << " " << Disk.leader_data_block[i].free_count << " ";
        for (int j = 0; j < DATAFREE; j++)
            cout << Disk.leader_data_block[i].free_array[j] << " ";
        cout << endl;
    }

    for (int i = 0; i < 10; i++)
    {
        cout << i << " ";
        for (int j = 0; j < BLOCKSIZ; j++)
            cout << Disk.member_data_block[i][j];
        cout << endl;
    }

    for (vector<login_user_table_item>::size_type i = 0; i < user_table.size(); i++)
    {
        cout << user_table[i].u_basic.u_category << " " << user_table[i].u_basic.u_gid << " " << user_table[i].u_basic.u_uid
             << " " << user_table[i].u_basic.u_password << " " << user_table[i].u_basic.u_logcount << endl;
        for (int j = 0; j < NOFILE; j++)
            cout << user_table[i].u_ftable[j] << " ";
        cout << endl;
    }

    for (int j = 0; j < DIRITEMNUM; j++)
    {
        cout << cur_dir.direct[j].dir_item_name << " " << cur_dir.direct[j].dir_item_ID << endl;
    }
}

void test_inode() //测试磁盘i节点、内存i节点的分配和释放
{
    i_node *put_i_node1, *put_i_node2;
    for (int i = 0; i < 513; i++)
    {
        i_node *a;
        a = i_alloc();
        if (a == nullptr)
            cout << "useless pointer" << endl;
        if (i + 1 == 257)
            put_i_node1 = a;
        if (i + 1 == 1)
            put_i_node2 = a;
    }

    for (unsigned int i = 1; i <= 510; i++)
    {
        i_free(i);
    }
    i_put(put_i_node1);
    i_put(put_i_node2);

    cout << "group i_node:" << endl;
    for (int i = 0; i < 11; i++)
    {
        cout << "number" << i + 1 << " ";
        for (int j = 0; j < IFREE; j++)
        {
            cout << Disk.leader_i_block[i].free_array[j] << " ";
        }
        cout << endl;
    }

    cout << "i_pointer: " << Disk.s_block.s_free_i_pointer << endl;
    for (int i = 0; i < NICINOD; i++)
    {
        cout << Disk.s_block.s_free_i_array[i] << " ";
    }
    cout << endl;

    cout << "hash table: " << endl;
    for (int i = 0; i < NHINO; i++)
    {
        i_node *temp;
        temp = i_table[i].i_pointer1;
        cout << i + 1 << " ";
        while (temp)
        {
            cout << temp->i_number << " ";
            temp = temp->i_pointer1;
        }
        cout << endl;
    }
}

int main()
{
    open_file_system();
    string cmd1, cmd2, cmd3;
    while (1)
    {
        print_cur_path();
        cin >> cmd1;
        if (cmd1 == "exit")
        {
            exit_file_system();
            continue;
        }
        else if (cmd1 == "login")
        { //登录
            user_login();
            continue;
        }
        else if (cmd1 == "switch")
        { //切换用户
            user_switch();
            continue;
        }
        else if (cur_user.u_basic.u_uid == 0)
        { //未登录或选择有效用户情况下禁止其他操作
            cout << "Please log in or switch effective users first!\n";
            continue;
        }
        else if (cmd1 == "resetpass")
        { //重置密码
            user_change_password();
            continue;
        }
        else if (cmd1 == "logout")
        { //注销
            user_logout();
            continue;
        }
        else if (cmd1 == "mkfs")
        { //格式化系统
            if (cur_user.u_basic.u_category != ADMIN)
            { //非管理员用户无法格式化系统
                cout << "ERROR : You have insufficient user rights! Please contact the administrator.\n";
                continue;
            }
            else if (user_table.size() > 1)
            { //有其他用户已登录时无法格式化系统
                cout << "ERROR : There are other users currently using the system!\n";
                continue;
            }
            else
            {
                string confirm;
                cout << "Confirm format?(y/n):";
                cin >> confirm;
                if (confirm == "y")
                {
                    cout << "Please wait while formatting";
                    disk_init();
                    Sleep(1000);
                    for (int i = 0; i < 3; i++)
                    {
                        cout << ".";
                        Sleep(1000);
                    }
                    cout << "\nDisk formatted successfully!\n";
                    open_file_system();
                }
                else
                    continue;
            }
        }
        else if (cmd1 == "dir")
        { //查看当前目录
            show_dir();
            continue;
        }
        else if (cmd1 == "cd")
        {
            cin >> cmd2;
            if (cmd2 == "..")
            { //返回上级目录
                if (cur_dir.this_dir_name == "root")
                { //在根目录环境下无法返回上级目录
                    cout << "The current root directory cannot be returned to the previous level!" << endl;
                    continue;
                }
                else
                {
                    cur_dir = last_dir_list[last_dir_list.size() - 1];
                    last_dir_list.pop_back();
                }
            }
            else
            { //打开当前路径下的目录
                unsigned short i_number = name_i(cmd2);
                if (i_number == 0)
                    cout << "ERROR: This directory does not exist! Failed to open directory.\n";
                else
                    change_dir(i_number, cmd2);
            }
        }
        else if (cmd1 == "find")
        { //查找文件及文件夹
            cin >> cmd2;
            search_file_or_dir(cmd2);
            continue;
        }
        else if (cmd1 == "mv")
        { //文件重命名
            cin >> cmd2 >> cmd3;
            rename(cmd2, cmd3);
            continue;
        }
        else if (cmd1 == "mkdir")
        {
            cin >> cmd2;
            make_dir(cmd2, 1);
            continue;
        }
        else if (cmd1 == "rmdir")
        {
            cin >> cmd2;
            delete_dir(cmd2);
            continue;
        }
        else if (cmd1 == "touch")
        {
            cin >> cmd2 >> cmd3;
            create_file(cmd2, atoi(cmd3.c_str()), 1);
            continue;
        }
        else if (cmd1 == "rm")
        {
            cin >> cmd2;
            delete_file(cmd2);
            continue;
        }
        else if (cmd1 == "open")
        {
            cin >> cmd2;
            open_file(cmd2);
            continue;
        }
        else if (cmd1 == "close")
        {
            cin >> cmd2;
            close_file(cmd2);
            continue;
        }
        else if (cmd1 == "cut")
        {
            cin >> cmd2;
            cut_file(cmd2);
            continue;
        }
        else if (cmd1 == "copy")
        {
            cin >> cmd2;
            copy_file(cmd2);
            continue;
        }
        else if (cmd1 == "paste")
        {
            paste_file();
            continue;
        }
        else if (cmd1 == "cat")
        {
            cin >> cmd2;
            read_file(cmd2);
            continue;
        }
        else if (cmd1 == "vi")
        {
            cin >> cmd2 >> cmd3;
            write_file(cmd2, cmd3);
            continue;
        }
    }
    return 0;
}