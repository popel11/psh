psh -  sandboxing shell

`psh` is a simple Unix shell that executes all programs under a restricted
user account to deny them access to your files.

This is first of a series of programs aiming to make malware less dangerous.
The other programs will be psh-compatible utilites that will not interact with
the filesystem, and rely on the shell to open any necessary files for them.

This is an early version, I'll improve.

DOC.txt may help you navigate through the source.

# Installation
0. Run `build.sh` to create executables `psh` and `restrict`.  
1. Move `restrict` to /bin and enable set-UID to root.  
2. you may also move `psh` to /bin, but that's optional.  

# Features

## Executables
The command `program arg1 arg2` causes a program `/bin/restricted` ro run with
arguments `/path/to/program`, `arg1`, `arg2` (the PATH environ variable is
used for path substitution, unless the path is already).

If the program name is preceded with a colon (`:program args`), it will run
as you - `/path/to/program` will run with `args`. This is used when you want
to give full privileges to a program.

## Built-in commands
- `cd path` sets current working directory to `path`  
- `read varname` assigns its whole standard input to the variable `varname`,
including whitespace.  
- `explain topic` gives detailed information about various topics,
from throwing knives to point of life.  

## Compound commands
Pipelines are formed by `|`, and/or lists by `&&` and `||`.
If a bracket is a member of a pipeline, it is executed in a child process, so
variable-sets within it do not affect the shell.

## Loops
Loop is formed by the `->` operator: `condition -> body` (where `condition`
and `body` are any commands or brackets).
There is currently no "break" and "continue". Adding them probably needs a
big refactor.

## Brackets
Any command may be enclosed in curly braces `{...}` to become a sub-command of
something else. The bracket runs in the current process, there is no way to
make a subshell.

## Variables
Variables can be set using `read` (see above). When a command contains
a dollar sign followed by a variable name (`$varname`), it is substituted for
a variable's value, or empty string if the variable was not set.
Names starting with an underscore refer to environment variable
(e. g. `$_PATH` refers to an environment variable named PATH).

Shell operators within variables are not interpreted, the variable's value
is always used as if quoted. This is an intentional security feature.

## Redirections
Any command may be redirected (although brackets are often needed, as
redirections have a low priority). Operators `[n]>file`, `[n1]>&n2`, `n<>file`
`n>>file`  work as in standard shells.

The operators `@>file`, `@>file`, `@>>file`, `@<>file` open `file` in the
respective mode to an arbitrary descriptor, and every executable for which this
redirection applies receives the descriptor number as an extra argument.
If used more than once, the arguments are in the order the redirections were
applied - brackets first, then left to right.  

Redirections must be at the end of arguments, because (0) it's easier to
process that way and (1) interleaving redirections with arguments seems
useless. If you want it, just tell me.  

# Future work
- add subshell brackets  
- use libreadline  
- add history  

# Author
**E-mail**: v.polak06@email.cz

**Age public key**:

> age1pq1kx28zyqmkvv7frhp8yyypx9eaneyltrze5mtj7emk4nz8r9ngngqrahnd8l6rtaxxwjf4xae7jnmlzpnz3dg2myzdj7l8eg5w5d5c67xtw4u8xlu4wjyggeshcpfxru34u482052kcavly4h23au3mene0hxh067hd6k4s2nxlftty3v5jl9sanuddz5ugmeftt4dc7k50efwps9e9dnwdcx6tc4vtdknxmqqxvew6gqs5mdpl2ung35x9rhxqxhkdmg6ej39un2lchrfx87n2l3rj2c3zf8a33f5der8mwk92fjxquvvkyh887p8hksnqdd270sl8ynsfx25zvr4as35hm202g9skreyj6n0sdexqz9cd6cqusm5284njpjepcfxl5g0q6tcj8d08y94ckfkqpcndtqsjrnwvy3c4cvh9xvaw9v5d0xdtuufw9j2pz85d4pzv5xc02rx5h7evacj5vhljulkvrgq98rhs5rfyw6xx7g3dzzrxt6wtzm8lu05dtzffculrzpetq4cre29f7zw83j2dvnfpcl5taph0xphyk2sqv2wqclwewfl04hejx699fgjsh5zaj6fgt5xk725h2gr4hc3nndcqpwm0p7x2xyqxm5fjht4d4evu9pwxth9ss2xeejccy22h2qdp8d84qesrrr5ycgfx3kc29yk9ushsjvmczuyn9hhnrktfr4g6hf9eszj79qx2528wzwgvmak7jp2yqyx8u3nrgkv4fp03j8dqvktermcejjfn5ep86s4dg6gfpllw6w2qpyvmtfjudvvzdy6ygttz4ay8tpl98rnqz6cnvnyekpa9vca86yy3nj8y0rgl4djcvuactr6fq9rajtc2mjy65jqgevy49aksnusdhenkwh3d2mqqvg6v6pkt49yra9ypsxwmtpx4c39gsfnafmt78xd7hmzcc563k7cfydtj4tlzufza4rzwx9pp4gppc89x3hhmnk22pd094fqqnw0wrv64mmv9ksnzr3dnfpxqd9d73guazk4wl43r2e3jw55jchewsp3gsxhe8kp46y3vlgepytlqapqca9u3d29kud2pgs63ejv2xg0pzeuy4shpmuyf76g7aeygucgp5tvrke47cyn9dlz64l7qdm00gepcsf0e88fg0lk4fcqwqxnt2msfkfcwk0cj2qwxq2w7wy6859v74hny8n92ar8yhhmafyau5v5ghne02ez5cdsss6s6rfz0mm65tu0u6axypz2eehmzry0fesqeh60jyrpdm5tvcpz6gj2jah0qyv4565zvqdcmydyu4kpnc94mexp95pfgq9dznejs4h67j30dpr2c8cx0p3edpe9ad76jq8h3arg9vg8f60w2vl4ymyzcvqs86955c53945xkscxxv3y7znxf8hy4egdrkme2v96c3euzrpcst2n9hcjxqanj85vujag4dgxec3e8nw0v0m77m5rgczpx2qrrh6w6efkuz8597ujvgacpd3jd2rz0yjjk6wghpsqja4jq6gm806pqwxf9rpca57a9zrl5d23c2t9gxpzrgy5xh67e78wnamg80fg7m05ftx3v3r6rrvvpjyaxhk0kvy25edhf0f8uk9c0xvmfh2t6w9vh267ckstzztkr3exv6jtncmhd5r33fefus33wnh37j8zspcyels89lt5gfj0has9sxn0dlwxxhhjvv4c5ar50m6dgmydle8e2trx4gfj39qddsneqerng3mw8qd709r4622ex65jpre9xypx2qsnznq0puh9w4mq9xg290y2dplsz8h256s492zu0atky2zrj3exjzzu346jht30fpkwmcn9ua3luj402lk676gg7xxrc79ek06huejzflkdtrpk2xg6anvhdzsem98l2sakkw73dnf6y2hczcudl7hhzdy0gl9g3fa
