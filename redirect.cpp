#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <regex.h>
#include "types.hpp"

#include "redirect.hpp"

#define UNSET_CLOEXEC(fd) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) & ~FD_CLOEXEC)

static fd_t throwing_open (const char *path, int mode)
{
	fd_t res;

	/* This doesn't set close-on-exec, since the program is meant
	 * to inherit it */
	if (mode & O_CREAT)
		res = open(path, mode, S_IRUSR | S_IWUSR);
	else
		res = open(path, mode);

	if (res < 0)
		throw strerror(errno);

	return res;
}

static void open_to_fd (const char *path, int mode, fd_t target_fd)
{
	fd_t tmpfd;

	/* not necessary, but may help get the descriptor right without
	 * duping. If it's not open, close() just fails. */
	close(target_fd);

	tmpfd = throwing_open(path, mode);

	if (tmpfd == target_fd) /* we got lucky */
		return;

	if (dup2(tmpfd, target_fd) == -1){
		close(tmpfd);
		throw strerror(errno);
	}
	close(tmpfd);
}

void RedirectList::add_open (std::string path, int mode, fd_t target)
{
	struct redirect rd;

	rd.target = target;
	rd.type = OPEN;
	rd.open_path = path;
	rd.open_mode = mode;

	redirs.push_back(rd);
}

void RedirectList::add_open_eval (std::string path, int mode)
{
	struct redirect rd;

	rd.type = OPEN_EVAL;
	rd.open_path = path;
	rd.open_mode = mode;

	redirs.push_back(rd);
}

void RedirectList::add_dup (fd_t source, fd_t target)
{
	struct redirect rd;

	rd.target = target;
	rd.type = DUP;
	rd.dup_fd = source;

	redirs.push_back(rd);
}

std::list<std::string> RedirectList::run () const
{
	std::list<std::string> added_args;
	fd_t fd;

	for (auto r: redirs){
		switch (r.type){
		case OPEN:
			open_to_fd(r.open_path.c_str(),
					r.open_mode,
					r.target);
			break;
		case OPEN_EVAL:
			fd = throwing_open(r.open_path.c_str(),
					r.open_mode);
			added_args.push_back(std::to_string(fd));
			break;

		case DUP:
			if (r.target == r.dup_fd){
				UNSET_CLOEXEC(r.target);
				break;
			}

			if (dup2(r.dup_fd, r.target) == -1)
				throw strerror(errno);
			break;
		}
	}

	return added_args;
}

void RedirectList::append (const RedirectList &other) /*inval*/
{
	for (auto r: other.redirs)
		redirs.push_back(r);
}

fd_t RedirectList::what_dupes_to (fd_t target) const noexcept
{
	fd_t fd = CLOSED_FILENO;

	/* Don't return on the first hit, because it may be overridden with
	 * redirections after it. */
	for (auto it = redirs.cbegin(); it != redirs.cend(); it++){
		if (it->target == target){
			if (it->type == DUP)
				fd = it->dup_fd;
			else
				fd = CLOSED_FILENO;
		}
	}

	return fd;
}

void RedirectList::dump (unsigned indent_depth) const
{
	for (auto rd: redirs){
		for (unsigned i = 0; i < indent_depth; i++)
			putc('\t', stderr);

		switch (rd.type){
		case DUP:
			fprintf(stderr, "Duplicate %d -> %d\n",
				rd.dup_fd, rd.target);
			break;
		case OPEN:
			fprintf(stderr, "Open '%s' -> %d\n",
				rd.open_path.c_str(), rd.target);
			break;
		case OPEN_EVAL:
			fprintf(stderr, "Open '%s' to any descriptor\n",
				rd.open_path.c_str());
			break;
		}
	}
}
