#ifndef TYPES_HPP
#define TYPES_HPP 1

#include <unistd.h>
#include <assert.h>
#include <regex.h>
#include <string.h>
#include <errno.h>

typedef int exitcode_t; /* Exit code of a sub-program or pipeline */
typedef int fd_t; /* File descriptor */

#define CLOSED_FILENO (-1) /* Null value of file descriptor */
#define NO_PROCESS ((pid_t) -1) /* Null value of process ID */

#define EXIT_SIGNALLED 130
#define EXIT_SYS_ERROR 127

/* See builtin.cpp and loop.cpp */
#define SHELL_BREAK 0
#define SHELL_CONTINUE 1

/* Some classes have a "terminating" method after which the object should be
 * no longer used. They may inherit from this class. */
class LifetimeGuarded {
protected:
	LifetimeGuarded (): expired(false) { }
	void assert_not_expired () const
	{
		assert(!expired);
	}
	void expire () { expired = true; }
	void assert_expired () const
	{
		assert(expired);
	}
	~LifetimeGuarded () { }

private:
	bool expired;
};

/* A variant of std::unique_ptr for file descriptors.
 * Closes it automatically when going out of scope. */
class OwnedFile {
public:
	OwnedFile (): fd(CLOSED_FILENO) { } /* close(-1) just fails */

	OwnedFile (fd_t _fd): fd(_fd) { }

	~OwnedFile ()
	{
		close(fd);
	}

	fd_t get () const
	{
		return fd;
	}
	void reset (fd_t new_fd)
	{
		close(fd);
		fd = new_fd;
	}
	fd_t release ()
	{
		fd_t res = fd;
		fd = CLOSED_FILENO;
		return res;
	}

private:
	fd_t fd;
};

#endif
