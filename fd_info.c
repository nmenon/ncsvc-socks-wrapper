#include <string.h>
#include <stdlib.h>

#include "preload.h"
#include "fd_info.h"
#include "hashtable.h"
#include "dbg.h"

static DEFINE_HASHTABLE(fds, 8);
static LIST_HEAD(listeners);

void fd_listener_add(const struct fd_listener *l)
{
	struct fd_listener *newl;

	newl = malloc(sizeof(*newl));
	memcpy(newl, l, sizeof(*l));
	list_add(&newl->node, &listeners);
}

static struct fd_info *fd_find(int fd)
{
	struct fd_info *info;
	hash_for_each_possible(fds, info, node, fd)
		if (info->fd == fd) return info;
	return NULL;
}

void fd_grab(int fd, const struct fd_listener *l)
{
	struct fd_info *info;
	dbg("%s(%d)\n", __func__, fd);
	info = malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));
	info->listener = (struct fd_listener *) l;
	info->fd = fd;
	hash_add(fds, &info->node, fd);
}

int fd_open(const char *pathname)
{
	struct fd_info *fd_info;
	struct fd_listener *l;
	int ret = FD_NONE;

	fd_info = malloc(sizeof(*fd_info));
	memset(fd_info, 0, sizeof(*fd_info));

	list_for_each_entry(l, &listeners, node) {
		if (!l->open) continue;
		ret = l->open(fd_info, pathname);
		if (ret == FD_NONE) continue;
		if (ret < 0) break;
		fd_info->listener = l;
		fd_info->fd = ret;
		hash_add(fds, &fd_info->node, ret);
		return ret;
	}
	free(fd_info);
	return ret;
}

int fd_socket(int domain, int type, int protocol)
{
	struct fd_info *fd_info;
	struct fd_listener *l;
	int ret = FD_NONE;

	fd_info = malloc(sizeof(*fd_info));
	memset(fd_info, 0, sizeof(*fd_info));

	list_for_each_entry(l, &listeners, node) {
		if (!l->socket) continue;
		ret = l->socket(fd_info, domain, type, protocol);
		if (ret == FD_NONE) continue;
		if (ret < 0) break;
		fd_info->listener = l;
		fd_info->fd = ret;
		hash_add(fds, &fd_info->node, ret);
		return ret;
	}
	free(fd_info);
	return ret;
}

int fd_ioctl(int fd, int request, char *argp)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->ioctl)
		ret = fd_info->listener->ioctl(fd_info, request, argp);

	return ret;
}

int fd_accept(int fd, struct sockaddr *addr, socklen_t *addr_len)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->accept)
		ret = fd_info->listener->accept(fd_info, addr, addr_len);

	return ret;
}

int fd_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->bind)
		ret = fd_info->listener->bind(fd_info, addr, len);

	return ret;
}

int fd_connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->connect)
		ret = fd_info->listener->connect(fd_info, addr, len);

	return ret;
}

int fd_listen(int fd, int n)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->listen)
		ret = fd_info->listener->listen(fd_info, n);

	return ret;
}

int fd_fstat(int fd, struct stat *stat_buf)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->fstat)
		ret = fd_info->listener->fstat(fd_info, stat_buf);

	return ret;
}

int fd_getsockopt(int fd, int level, int name, void *val, socklen_t *len)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->getsockopt)
		ret = fd_info->listener->getsockopt(fd_info, level, name, val, len);

	return ret;
}

int fd_setsockopt(int fd, int level, int name, const void *val, socklen_t len)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->setsockopt)
		ret = fd_info->listener->setsockopt(fd_info, level, name, val, len);

	return ret;
}

int fd_getsockname(int fd, struct sockaddr *addr, socklen_t *len)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->getsockname)
		ret = fd_info->listener->getsockname(fd_info, addr, len);

	return ret;
}

ssize_t fd_recv(int fd, void *buf, size_t len, int flags)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->recv)
		ret = fd_info->listener->recv(fd_info, buf, len, flags);
	return ret;
}

ssize_t fd_send(int fd, const void *buf, size_t len, int flags)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->send)
		ret = fd_info->listener->send(fd_info, buf, len, flags);
	return ret;
}

ssize_t fd_recvfrom(int fd, void *buf, size_t len, int flags,
			struct sockaddr *src_addr, socklen_t *addrlen)
{
	struct fd_info *fd_info;
	int ret = FD_NONE;

	fd_info = fd_find(fd);
	if (fd_info && fd_info->listener->recvfrom)
		ret = fd_info->listener->recvfrom(fd_info, buf, len, flags,
						src_addr, addrlen);
	return ret;
}

void fd_close(int fd)
{
	struct fd_info *fd_info;

	fd_info = fd_find(fd);
	if (fd_info) {
		if (fd_info->listener->close)
			fd_info->listener->close(fd_info);
		hash_del(&fd_info->node);
		free(fd_info);
	}
}
