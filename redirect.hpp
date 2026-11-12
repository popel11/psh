#ifndef REDIRECT_HPP
#define REDIRECT_HPP 1

#include <list>
#include <string>
#include "types.hpp"

enum redir_type {
	OPEN, /* TARGET should be opened by open() */
	OPEN_EVAL, /* The opened descriptor should be returned. See DOC.txt */
	DUP, /* TARGET should be duped from another descriptor */
	/* For DUP, if TARGET is the same as SOURCE, the child inherits
	 * that descriptor. There is not a separate INHERIT type, because
	 * shell commands often don't know they're passing
	 * identical TARGET and SOURCE. */
};

struct redirect {
	fd_t target;

	enum redir_type type;

	std::string open_path; /* for OPEN */
	int open_mode;

	fd_t dup_fd; /* for DUP */
};

/* Info on how file descriptors should be redirected for a sub-program.
 * The redirections are executed in the order they were added. */
class RedirectList {
public:
	RedirectList () { }
	RedirectList (const RedirectList &src): redirs(src.redirs) { }

	void add_open (std::string path, int mode, fd_t target);
	void add_open_eval (std::string path, int mode);

	/* TARGET may be the same as SOURCE, see above. */
	void add_dup (fd_t source, fd_t target);
	void add_inherit (fd_t what) { add_dup(what, what); }

	void append (const RedirectList &other);

	/* This is used in a child process (after fork(2), but before execv(2))
	 * to execute the redirections.
	 * Return list of OPEN_EVAL numbers to pass to the executed program. */
	std::list<std::string> run () const;

	fd_t what_dupes_to (fd_t target) const noexcept;

	void dump (unsigned indent_depth) const;

private:
	std::list<struct redirect> redirs;
};

#endif
