#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/tcp.h>

#include "ini.hpp"
#include "wan_share.hpp"
#include "thread_pool.hpp"

#define DEBUG 1
#define DEBUG_DEEP 0


#define PKG_MAX 10240
#define RE_SIZE 128
#define PATH_SIZE 128

int WAN_SHARE_SWITCH = 0;

struct task_t {
	int foruid;
	int fid;
	int port;
	char host[PATH_SIZE];
};

//配置文件中读取的参数
char *server_ip;
int server_port;
int heartbeat_interval = 30;
int max_thread_num = 5;
char *share_path;


time_t select_timeout = 1;
pthread_mutex_t wan_share_switch_mutex;

#define EXPANDED_NAME 10
char media_file_flag[][EXPANDED_NAME] = {
"wmv", "avi", "mpg", "mp4", "3gp", "wma", "rmvb", "rm", 	//视频
"gif", "mkv", "vob", "mov", "flv", "swf", "dv", "asf",		//视频
"ts", "dat", "f4v", "webm", 					//视频
"mp3", "wma", "ape", "flac", "aac", "mmf", "amr", "m4a", 	//音频
"m4r", "ogg", "wav", "mp2", "ac3", "ra", "au", 			//音频
"jpg", "png", "ico", "bmp", "gif", "tif", "pcx", "tga",		//图片
};

//正则表达式
char notify_re[] = "\\(NTFY\\) / HTTP/1.1\r\n.*\r\n.*\r\nUid: \\([0-9]*\\)\r\nForUid: *\\([0-9]*\\)\r\nFid: *\\([0-9]*\\)\r\nServerHost: \\(.*\\)\r\nServerPort: \\([0-9]*\\)\r\n.*";
char get_re[] = "\\(GET\\) /\\([a-z]*\\)/\\([0-9]*\\)/\\([0-9]*\\)\\(/.*\\) HTTP";
//char range_re_old[] = ".*\r\n.*\r\n.*\r\nRange: bytes=\\([0-9]*\\)-\r\n";
char range_re[] = ".*Range: bytes=\\([0-9]*\\)-.*";

//传进该模块的参数
int Uid;

char HTTP_LOGIN_MESS[] = 
"LGIN / HTTP/1.1\r\n\
HOST: %s:%d\r\n\
Event: Subsrcibe\r\n\
Uid: %d\r\n\
ForUid: %d\r\n\
Fid: %d\r\n\
Connection: keepalive\r\n\
Content-Length: 0\r\n\
\r\n";

char HTTP_LOGIN_DATA[] = 
"LGIN / HTTP/1.1\r\n\
HOST: %s:%d\r\n\
Event: Connect\r\n\
Uid: %d\r\n\
ForUid: %d\r\n\
Fid: %d\r\n\
Connection: keepalive\r\n\
Content-Length: 0\r\n\
\r\n";

char HTTP_HEARTBEAT[] = 
"KEEP / HTTP/1.1\r\n\
Event: Heartbeat\r\n\
Uid: %d\r\n\
Content-Length: 0\r\n\
\r\n";

char HTTP_BROWSE_HEAD[] = 
"HTTP/1.1 200 OK\r\n\
Content-Type: text/xml; charset=\"utf-8\"\r\n\
Connection: close\r\n\
Content-Length: %d\r\n\
\r\n";

char HTTP_BROWSE_CONTENT_FRONT[] =
"<?xml version=\"1.0\"?><body><medialist>";

char HTTP_BROWSE_CONTENT[] = 
"<mediaitem><type>%s</type><filename>%s</filename><size>%d</size></mediaitem>";
//     <mediaitem><type>file/dir</type><filename>33.mp3</filename><size>1024M<size></mediaitem>

char HTTP_BROWSE_CONTENT_LAST[] =
"</medialist></body>";


char HTTP_TRANSFER[] = 
"HTTP/1.1 206 OK\r\n\
Connection: close\r\n\
Content-Type: video/mp4\r\n\
Content-Length: %d\r\n\
Content-Range: bytes %d-%d/%d\r\n\
Accept-Ranges: bytes \r\n\
\r\n";

char TEST_NTFY[] = 
"NTFY / HTTP/1.1\r\n\
Host: 192.168.7.88:54321\r\n\
Event: Connect\r\n\
Uid: 1002\r\n\
ForUid: 1001\r\n\
Fid: 3\r\n\
ServerHost: 192.168.7.69\r\n\
ServerPort: 8090\r\n\
Connection: keepalive\r\n\
Content-Length: 0\r\n\
\r\n";

char TEST_BROWSE[] = "\
GET /browse/1001/1002/ HTTP/1.1\r\n\
HOST: to_clientip:to_clientport\r\n\
Content-Type: text/xml; charset=\"utf-8\"\r\n\
Content-Length: 0\r\n\
CONNECTION: close\r\n\
USER-AGENT: Linux/3.8.13.13-cdos, UPnP/1.0, Portable SDK for UPnP devices/1.6.17\r\n\
\r\n";

char TEST_DOWNLOAD[] = "\
GET /transfer/1001/1002/dlna/chongshangyunxiao.mkv HTTP/1.1\r\n\
HOST: client1002ip:client1002port\r\n\
Content-Length: 0\r\n\
Range: bytes=100-\r\n\
CONNECTION: close\r\n\
\r\n";


static char *substr (char *str_match, const char *str_raw, unsigned start, unsigned end)
{
	unsigned n = end - start;
	strncpy (str_match, str_raw + start, n);
	str_match[n] = 0;
	return str_match;
}

static void init_servaddr(struct sockaddr_in *addr, char *server_ip, int server_port)
{
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(server_port);
	if( inet_pton(AF_INET, server_ip, &(addr->sin_addr)) <= 0){
		printf("inet_pton error for %s\n", server_ip);
	}
}

static void init_login_pkg(char *pkg, char *pkg_template, int foruid, int fid)
{
	snprintf(pkg, PKG_MAX, pkg_template, server_ip, server_port, Uid, foruid, fid);
#if DEBUG
	printf("init_login_pkg(): \n%s", pkg);
#endif
}

static int parse_get(char *recv_pkg, char *path, long *range)
{
	char err_buf[256];
	int ret;
	regex_t reg;
	if ((ret = regcomp(&reg, get_re, 0)) != 0) {
		printf("parse_get(): regcomp error\n");
		regerror(ret ,&reg, err_buf, sizeof (err_buf));
		fprintf(stderr, "%s: pattern '%s' \n", err_buf, get_re);
		return -1;
	}

	regmatch_t pm[10];
	size_t nmatch = 10;
	if ((ret = regexec(&reg, recv_pkg, nmatch, pm, 0)) != 0) {
		printf("parse_get(): regexec error\n");
		regerror(ret ,&reg, err_buf, sizeof (err_buf));
		printf("raw string:\n%s", recv_pkg);
		fprintf(stderr, "%s: pattern '%s' \n", err_buf, get_re);
		return -1;
	}

	char method_match[RE_SIZE];
	char request_match[RE_SIZE];
	char formuid_match[RE_SIZE];
	char touid_match[RE_SIZE];
	char path_match[RE_SIZE];
#if DEBUG_DEEP
	printf("start:[%d], end: [%d]\n", pm[0].rm_so, pm[0].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[1].rm_so, pm[1].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[2].rm_so, pm[2].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[3].rm_so, pm[3].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[4].rm_so, pm[4].rm_eo);
	printf("strlen(notify): [%ld]\n", strlen(TEST_NTFY));
#endif
	substr(method_match, recv_pkg, pm[1].rm_so, pm[1].rm_eo);
	substr(request_match, recv_pkg, pm[2].rm_so, pm[2].rm_eo);
	substr(formuid_match, recv_pkg, pm[3].rm_so, pm[3].rm_eo);
	substr(touid_match, recv_pkg, pm[4].rm_so, pm[4].rm_eo);
	substr(path_match, recv_pkg, pm[5].rm_so, pm[5].rm_eo);
#if DEBUG
	printf("-------------------parse_get------------------\n");
	printf("method_match: [%s]\n", method_match);
	printf("request_match: [%s]\n", request_match);
	printf("formuid_match: [%s]\n", formuid_match);
	printf("touid_match: [%s]\n", touid_match);
	printf("path_match: [%s]\n", path_match);
	printf("-------------------parse_get------------------\n");
#endif
	strcpy(path, path_match);
	regfree(&reg);

	if (strcmp(request_match, "browse") == 0) {
		return 1;
	} else if (strcmp(request_match, "transfer") == 0) {
		if (strstr(recv_pkg, "Range") == NULL) {
			*range = 0;
		} else {
			if ((ret = regcomp(&reg, range_re, 0)) != 0) {
				printf("parse_get(): regcomp error\n");
				regerror(ret ,&reg, err_buf, sizeof (err_buf));
				fprintf(stderr, "%s: pattern '%s' \n", err_buf, get_re);
				return -1;
			}
		
			if ((ret = regexec(&reg, recv_pkg, nmatch, pm, 0)) != 0) {
				printf("parse_get(): regexec error\n");
				regerror(ret ,&reg, err_buf, sizeof (err_buf));
				printf("raw string:\n%s", recv_pkg);
				fprintf(stderr, "%s: pattern '%s' \n", err_buf, range_re);
				return -1;
			}
			char range_match[RE_SIZE];
			substr(range_match, recv_pkg, pm[1].rm_so, pm[1].rm_eo);
			printf("parse_get(): range: [%s]\n", range_match);
			*range = atol(range_match);
		}
		return 2;
	} else {
		printf("parse_get(): request error: [%s]\n", request_match);
		return -1;
	}
}

static void free_dirent(struct dirent **ent, int n)
{
	if (n < 0) {
		return;
	}
	while (n--) {
		free(ent[n]);
	}
	free(ent);
}

static int filter(const struct dirent *dir)
{
	struct stat buf;
	char absolute_path[PATH_SIZE];
	snprintf(absolute_path, PATH_SIZE, "%s/%s", share_path, dir->d_name);
	lstat(absolute_path, &buf);
	if (S_ISDIR(buf.st_mode)) {
		return 0;
	}
	char *en = NULL;
	int i;
	char file_name[128];
	strcpy(file_name, dir->d_name);
	en = strrchr(file_name, '.');
	if ( en == NULL ) {
		return 0;
	} else {
		en = en + 1;
		for (i = 0; i < (sizeof(media_file_flag) / EXPANDED_NAME); i++) {
			if (strcmp(en, media_file_flag[i]) == 0) {
				return 1;
			}
		}
	}
//		printf("%s\n", tmp_path_name);
	return 0;
}

static int browse_media_file(int sockfd, char *path)
{
	char absolute_path[PATH_SIZE];
	snprintf(absolute_path, PATH_SIZE, "%s/%s", share_path, path);
//	printf("browse_media_file(): absolute path: [%s]\n", absolute_path);

	int n;
	struct dirent **ent = NULL;
	n = scandir(absolute_path, &ent, filter, NULL);
//	printf("browse_media_file(): n: [%d]\n", n);

	int i;
	char tmp_path_name[PATH_SIZE];
	char content_pkg[PKG_MAX];
	memset(content_pkg, 0, PKG_MAX);
	char file_info[PKG_MAX];
	struct stat buf;
	strcat(content_pkg, HTTP_BROWSE_CONTENT_FRONT);
	for (i = 0; i < n; i++) {
		snprintf(tmp_path_name, PATH_SIZE, "%s/%s", absolute_path, ent[i]->d_name);
//		printf("%s\n", tmp_path_name);
		lstat(tmp_path_name, &buf);
		if (S_ISDIR(buf.st_mode)) {
			snprintf(file_info, PATH_SIZE, HTTP_BROWSE_CONTENT, \
				"dir", ent[i]->d_name, buf.st_size/1024);
			strcat(content_pkg, file_info);
		} else if (S_ISREG(buf.st_mode)) {
			snprintf(file_info, PATH_SIZE, HTTP_BROWSE_CONTENT, \
				"file", ent[i]->d_name, buf.st_size/1024);
			strcat(content_pkg, file_info);
		}
	}
	strcat(content_pkg, HTTP_BROWSE_CONTENT_LAST);
	free_dirent(ent, n);
//	printf("browse_media_file(): content_pkg :\n[%s]", content_pkg);
	char browse_pkg[PKG_MAX];
	snprintf(browse_pkg, PKG_MAX, HTTP_BROWSE_HEAD, strlen(content_pkg));

	strcat(browse_pkg, content_pkg);
	int send_len = 0;
	if ( (send_len = send(sockfd, browse_pkg, strlen(browse_pkg), 0)) < 0) {
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
#if DEBUG
	printf("browse_media_file(): browse_pkg :\n%s", browse_pkg);
	printf("browse_media_file(): sockfd: [%d] \n", sockfd);
	printf("browse_media_file(): send_len: [%d] \n", send_len);
	printf("browse_media_file(): strlen(browse_pkg) :%ld\n", strlen(browse_pkg));
#endif
	return 0;
}

static void transfer_media_file(int sockfd, char *path, long range)
{
	char absolute_path[PATH_SIZE];
	snprintf(absolute_path, PATH_SIZE, "%s/%s", share_path, path);

	struct stat buf;
	lstat(absolute_path, &buf);

	char transfer_pkg[PKG_MAX];
	snprintf(transfer_pkg, PKG_MAX, HTTP_TRANSFER, \
		buf.st_size - range, range, buf.st_size - 1, buf.st_size);
	long send_len;
	if ((send_len = send(sockfd, transfer_pkg, strlen(transfer_pkg), 0)) < 0) {
		printf("transfer_media_file() :send error: %s(errno: %d)\n", strerror(errno), errno);
	}
#if DEBUG
	printf("transfer_media_file(): sockfd: [%d] \n", sockfd);
	printf("transfer_media_file(): head send_len: [%ld] \n", send_len);
	printf("transfer_media_file(): file(absolute path): [%s], st_size: [%ld]\n", \
		absolute_path, buf.st_size);
	printf("transfer_media_file(): http_transfer: \n%s", transfer_pkg);
#endif
	int fd = open(absolute_path, O_RDONLY);
	lseek(fd, range, SEEK_SET);
	int read_len;
	while (1) {
		if ((read_len = read(fd, transfer_pkg, PKG_MAX)) == 0) {
			break;
		}
		if ((send_len = send(sockfd, transfer_pkg, read_len, MSG_NOSIGNAL)) != read_len) {
			printf("transfer_media_file() :send error: %s(errno: %d)\n", \
				strerror(errno), errno);
				goto end;
		}
#if DEBUG
		printf("transfer_media_file(): content send_len: [%ld] \n", send_len);
#endif
	}
end:
	close(fd);
}

static int deal_task(struct task_t *arg)
{
	int len = 0;
	int sockfd;
	int opt = 0;
	int option = 1;
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("deal_task(): create socket error: %s(errno: %d)\n", strerror(errno), errno);
		goto end;
	}
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&option, \
		(socklen_t)strlen((const char*)&option));
#if 0
	//对sock_cli设置KEEPALIVE和NODELAY
	len = sizeof(unsigned int);
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, (socklen_t)len);//使用KEEPALIVE
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void*)&optval, (socklen_t)len);
#endif
#if 0
	struct linger Linger;
	Linger.l_onoff = 1;// 启用
	Linger.l_linger = 5;// 缓冲区有数据时允许延迟5秒，没数据时忽略
	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const void*)&Linger, sizeof(struct linger));
#endif
#if 0
	int sndbuf=0;        /* Send buffer size */
	socklen_t optlen;        /* Option length */
	optlen = sizeof(sndbuf);
	getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &optlen);
	printf("deal_task(): SO_SNDBUF: [%d]\n", sndbuf);
#endif
	struct sockaddr_in servaddr;
	init_servaddr(&servaddr, arg->host, arg->port);

	if ( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		goto end;
	}

	char login_pkg[PKG_MAX];
	init_login_pkg(login_pkg, HTTP_LOGIN_DATA, arg->foruid, arg->fid);
	long send_len;
	if ( (send_len = send(sockfd, login_pkg, strlen(login_pkg), 0)) < 0 ) {
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		goto end;
	}
#if DEBUG
	printf("deal_task(): send LOGIN: send_len: [%ld]\n", send_len);
#endif

	char recv_pkg[PKG_MAX];
	if((len = recv(sockfd, recv_pkg, PKG_MAX, 0)) == -1) {
		printf("deal_task(): recv error: %s(errno: %d)\n", \
			strerror(errno), errno);
		goto end;
	}
#if DEBUG
	printf("deal_task(): sockfd: [%d] \n", sockfd);
	printf("deal_task(): \n%s", recv_pkg);
#endif
	
	/* 解析 get：
	1.判断任务类型（browse or download）
	2.获取request的参数：
	1）browse: path
	2）download: path/file		*/
	char path[PATH_SIZE];
	long range;
	opt = parse_get(recv_pkg, path, &range);
	if (opt == 1) {
		if (browse_media_file(sockfd, path) < 0) {
			goto end;
		}
	} else if (opt == 2) {
		transfer_media_file(sockfd, path, range);
	} else {
		goto end;
	}

end:
#if 1
	sleep(2);
#endif
	if (close(sockfd) < 0) {
		printf("close error: %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	return 0;
}

static int parse_notify(char *recv_pkg, int *foruid, int *fid, char *host, int *port)
{
	char err_buf[256];
	int ret;
	regex_t reg;
	if ((ret = regcomp(&reg, notify_re, 0)) != 0) {
		printf("parse_notify(): regcomp error\n");
		regerror(ret ,&reg, err_buf, sizeof (err_buf));
		fprintf(stderr, "%s: pattern '%s' \n", err_buf, notify_re);
		return -1;
	}

	regmatch_t pm[10];
	size_t nmatch = 10;
	if ((ret = regexec(&reg, recv_pkg, nmatch, pm, 0)) != 0) {
		printf("parse_notify(): regexec error\n");
		regerror(ret ,&reg, err_buf, sizeof (err_buf));
		printf("raw string:\n%s", recv_pkg);
		fprintf(stderr, "%s: pattern '%s' \n", err_buf, notify_re);
		return -1;
	}

	char notify_match[RE_SIZE];
	char uid_match[RE_SIZE];
	char foruid_match[RE_SIZE];
	char fid_match[RE_SIZE];
	char host_match[RE_SIZE];
	char port_match[RE_SIZE];
#if DEBUG_DEEP
	printf("start:[%d], end: [%d]\n", pm[0].rm_so, pm[0].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[1].rm_so, pm[1].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[2].rm_so, pm[2].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[3].rm_so, pm[4].rm_eo);
	printf("start:[%d], end: [%d]\n", pm[5].rm_so, pm[5].rm_eo);
	printf("strlen(notify): [%ld]\n", strlen(TEST_NTFY));
#endif
	substr(notify_match, recv_pkg, pm[1].rm_so, pm[1].rm_eo);
	substr(uid_match, recv_pkg, pm[2].rm_so, pm[2].rm_eo);
	substr(foruid_match, recv_pkg, pm[3].rm_so, pm[3].rm_eo);
	substr(fid_match, recv_pkg, pm[4].rm_so, pm[4].rm_eo);
	substr(host_match, recv_pkg, pm[5].rm_so, pm[5].rm_eo);
	substr(port_match, recv_pkg, pm[6].rm_so, pm[6].rm_eo);
	
#if DEBUG
	printf("-------------------parse_notify------------------\n");
	printf("notify_match: [%s]\n", notify_match);
	printf("uid_match: [%s]\n", uid_match);
	printf("foruid_match: [%s]\n", foruid_match);
	printf("fid_match: [%s]\n", fid_match);
	printf("host_match: [%s]\n", host_match);
	printf("port_match: [%s]\n", port_match);
	printf("-------------------parse_notify------------------\n");
#endif

	if (strcmp(notify_match, "NTFY") != 0) {
		printf("parse_notify(): method is [%s], not NTFY\n", notify_match);
		return -1;
	}
	if (atoi(uid_match) != Uid) {
		printf("parse_notify(): Uid is [%s], not %d\n", uid_match, Uid);
		return -1;
	}
	*foruid = atoi(foruid_match);
	*fid = atoi(fid_match);
	strcpy(host, host_match);
	*port = atoi(port_match);

	regfree(&reg);

	return 0;
}

static int receive_task(const int sockfd)
{
	int len = 0;
	fd_set fds;
	struct timeval timeout = {select_timeout, 0};
	int maxfd = sockfd + 1;
	char recv_pkg[PKG_MAX];
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	if (select(maxfd, &fds, NULL, NULL, &timeout) > 0) {
		if(FD_ISSET(sockfd, &fds))
		{
			if((len = recv(sockfd, recv_pkg, PKG_MAX,0)) == -1) {
				printf("receive_task(): recv error: %s(errno: %d)\n", \
					strerror(errno), errno);
				return -1;
			}
#if DEBUG
			printf("receive_task(): \n%s", recv_pkg);
#endif
			//解析服务器发来的通知 & 初始化参数arg
			struct task_t arg;
			if (parse_notify(recv_pkg, &(arg.foruid), &(arg.fid), arg.host, &(arg.port)) < 0) {
				return -1;
			}

			//添加到线程池的任务链表
			tpool_add_work((void* (*)(void*))deal_task, (void*)&arg);
		}
	}
	return 0;
}

static int heartbeat(int sockfd, time_t* last_heartbeat_time)
{
	time_t current_time = time(NULL);
	if (current_time - *last_heartbeat_time > heartbeat_interval) {
		char heartbeat_pkg[PKG_MAX];
		snprintf(heartbeat_pkg, PKG_MAX, HTTP_HEARTBEAT, Uid);
		long send_len = 0;
		if( (send_len = send(sockfd, heartbeat_pkg, strlen(heartbeat_pkg), 0) ) < 0) {
			printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
			return -1;
		}
#if DEBUG
		printf("heartbeat(): send heartbeat, send_len: [%ld]\n", send_len);
#endif
		*last_heartbeat_time = current_time;
	}
	return 0;
}

static int check_wan_share_switch(void)
{
	pthread_mutex_lock(&wan_share_switch_mutex);
	if (WAN_SHARE_SWITCH == 1) {
		pthread_mutex_unlock(&wan_share_switch_mutex);
//		printf("sub thread: continue working\n");
		return 0;
	} else {
		pthread_mutex_unlock(&wan_share_switch_mutex);
//		printf("sub thread: switch close\n");
		return -1;
	}
}

static void* wan_share(void* arg)
{
	//登录
	int cmd_sockfd;
	int option = 1;
	time_t last_heartbeat_time = 0;
	if( (cmd_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		goto err1;
	}
	setsockopt(cmd_sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&option, \
		(socklen_t)strlen((const char*)&option));

	struct sockaddr_in servaddr;
	init_servaddr(&servaddr, server_ip, server_port);

	if( connect(cmd_sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		goto err1;
	}

	char login_pkg[PKG_MAX];
	init_login_pkg(login_pkg, HTTP_LOGIN_MESS, 0, 0);
	long send_len;
	if( (send_len = send(cmd_sockfd, login_pkg, strlen(login_pkg), 0)) < 0 ) {
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		goto err1;
	}
#if DEBUG
	printf("wan_share(): send LOGIN: send_len: [%ld]\n", send_len);
#endif

	//初始化线程池
	if (tpool_create(max_thread_num) != 0) {
		printf("tpool_create failed\n");
		goto err2;
	}

	while (1) {
		//接收服务器发送来的请求，分配任务到线程池中的线程
		if (receive_task(cmd_sockfd) < 0) {
			goto err2;
		}

		//心跳
		if (heartbeat(cmd_sockfd, &last_heartbeat_time) < 0) {
			goto err2;
		}

		//如果广域网共享关闭，则断开和服务器的连接、销毁线程池、退出线程
		if (check_wan_share_switch() < 0) {
			goto end;
		}
	}
end:
err2:
	tpool_destroy();
err1:
	pthread_mutex_destroy(&wan_share_switch_mutex);
	close(cmd_sockfd);
	close_wan_share();
	pthread_exit(NULL);
}

static int init_config(void)
{
	ini_t *conf = ini_load("vlc.conf");
	if (conf == NULL) { 
		printf("init_config(): %s(errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	ini_read_str(conf, "wan_share", "server_ip", &server_ip, NULL);
	ini_read_int(conf, "wan_share", "server_port", &server_port, 0);
	ini_read_int(conf, "wan_share", "heartbeat_interval", &heartbeat_interval, 0);
	ini_read_int(conf, "wan_share", "max_thread_num", &max_thread_num, 0);
	ini_read_str(conf, "wan_share", "share_path", &share_path, NULL);
#if DEBUG
	printf("init_config(): server_ip: [%s]\n", server_ip);
	printf("init_config(): server_port: [%d]\n", server_port);
	printf("init_config(): heartbeat_interval: [%d]\n", heartbeat_interval);
	printf("init_config(): max_thread_num: [%d]\n", max_thread_num);
	printf("init_config(): share_path: [%s]\n", share_path);
#endif
	ini_free(conf);
	return 0;
}

int open_wan_share(int uid)
{
	if (init_config() < 0) {
		return -1;
	}

	pthread_t wan_share_tid;
	if (pthread_mutex_init(&wan_share_switch_mutex, NULL) != 0) {
		printf("init wan_share_switch_mutex error: %s(errno: %d)\n", strerror(errno), errno);
		goto err;
	}
	pthread_mutex_lock(&wan_share_switch_mutex);
	WAN_SHARE_SWITCH = 1;
	pthread_mutex_unlock(&wan_share_switch_mutex);

	Uid = uid;
	if (pthread_create(&wan_share_tid, NULL, wan_share, NULL) != 0) {
		printf("Create wan_share thread failed\n");
		goto err;
	}
	if (pthread_detach(wan_share_tid) != 0) {
		printf("Detach wan_share thread failed\n");
		goto err;
	}
	return 0;

err:
	return -1;
}

void close_wan_share(void)
{
	pthread_mutex_lock(&wan_share_switch_mutex);
	WAN_SHARE_SWITCH = 0;
	pthread_mutex_unlock(&wan_share_switch_mutex);
}

int read_wan_share_status(void)
{
	int status;
	pthread_mutex_lock(&wan_share_switch_mutex);
	status = WAN_SHARE_SWITCH;
	pthread_mutex_unlock(&wan_share_switch_mutex);
	return status;
}

int main()
{
//	printf("%s", HTTP_LOGIN);

	if (open_wan_share(1103) < 0) {
		exit(-1);
	}
	sleep(9999999);

	close_wan_share();
	return 0;
}


