#!/bin/bash

# ----------------------------------------------------------------- 

RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
RESET="\033[0m"
OK=$GREEN"OK"$RESET
NG=$RED"NG"$RESET

cat <<EOF | gcc -xc -o a.out -
#include <stdio.h>
int main() { printf("hello from a.out\n"); }
EOF

cat <<EOF | gcc -xc -o print_args -
#include <stdio.h>
int main(int argc, char *argv[]) {
	for (int i = 0; argv[i]; i++)
		printf("argv[%d] = [%s]\n", i, argv[i]);
}
EOF

cat <<EOF | gcc -xc -o exit42 -
int main() { return 42; }
EOF

print_desc() {
	echo -e $YELLOW"$1"$RESET
}

cleanup() {
	rm -f cmp out a.out print_args exit42 infinite_loop no_exec_perm no_read_perm
}

assert() {
	COMMAND="$1"
	shift
	printf '%-70s:' "[$COMMAND]"
	# exit status
	echo -n -e "$COMMAND" | bash >cmp 2>&-
	expected=$?
	for arg in "$@"
	do
		mv "$arg" "$arg"".cmp"
	done
	echo -n -e "$COMMAND" | ./minishell >out 2>&-
	actual=$?
	for arg in "$@"
	do
		mv "$arg" "$arg"".out"
	done

	if diff out cmp > /dev/null; then
		echo -e -n "  diff $OK"
	else
		echo -e -n "  diff $NG"
		printf '%-70s:\n' "[$COMMAND]" >> error.log
		diff -U 1 out cmp >>error.log
	fi

	if [ "$actual" = "$expected" ]; then
		echo -e -n "  status $OK"
	else
		echo -e -n "  status $NG, expected $expected but got $actual"
		printf '%-70s:' "[$COMMAND]" >>error.log
		echo "status NG, expected $expected but got $actual" >>error.log
	fi
	for arg in "$@"
	do
		echo -n "  [$arg] "
		diff "$arg"".cmp" "$arg"".out" >/dev/null && echo -e -n "$OK" || echo -e -n "$NG"
		rm -f "$arg"".cmp" "$arg"".out"
	done
	echo
}

rm -f error.log

# ----------------------------------------------------------------- 

# STEP1
# Empty line(EOF)
assert ''

# STEP2
# bin command
assert '/bin/pwd'
assert '/bin/echo'
assert '/bin/ls'

#STEP3
# PATH command
assert 'pwd'
assert 'echo'
assert 'ls'
assert './a.out'
# no such command
assert 'a.out'
assert 'nosuchfile'

#STEP4
# Tokenize
## unquoted word
assert 'ls /'
assert 'echo hello    world     '
assert 'nosuchfile\n\n'

## single quote
assert "./print_args 'hello   world' '42Tokyo'"
assert "echo 'hello   world' '42Tokyo'"
assert "echo '\"hello   world\"' '42Tokyo'"

## double quote
assert './print_args "hello   world" "42Tokyo"'
assert 'echo "hello   world" "42Tokyo"'
assert "echo \"'hello   world'\" \"42Tokyo\""

## combination
assert "echo hello'      world'"
assert "echo hello'  world  '\"  42Tokyo  \""

## command not found
assert '""'
# assert '.' # . is a builtin command in bash
assert '..'

## is a directory
assert './'
assert '/'
assert '/etc'
assert '/etc/'
assert '////'

## Permission denied
echo "int main() { }" | gcc -xc -o no_exec_perm -
chmod -x no_exec_perm
assert 'no_exec_perm'
assert './no_exec_perm'
echo "int main() { }" | gcc -xc -o no_read_perm -
chmod -r no_read_perm
assert 'no_read_perm'
assert './no_read_perm'

mkdir -p /tmp/a /tmp/b
echo "int main() { return 1; }" | gcc -xc -o /tmp/a/simple_test -
echo "int main() { return 2; }" | gcc -xc -o /tmp/b/simple_test -

print_desc "/tmp/a /tmp/b both with permission"
assert 'unset PATH\nexport PATH="/tmp/a:/tmp/b"\nsimple_test'
assert 'unset PATH\nexport PATH="/tmp/b:/tmp/a"\nsimple_test'

print_desc "/tmp/a /tmp/b both without permission"
chmod -x /tmp/a/simple_test; chmod -x /tmp/b/simple_test;
assert 'unset PATH\nexport PATH="/tmp/a:/tmp/b"\nsimple_test'
assert 'unset PATH\nexport PATH="/tmp/b:/tmp/a"\nsimple_test'

print_desc "a with permission, b without permission"
chmod +x /tmp/a/simple_test; chmod -x /tmp/b/simple_test;
assert 'unset PATH\nexport PATH="/tmp/a:/tmp/b"\nsimple_test'
assert 'unset PATH\nexport PATH="/tmp/b:/tmp/a"\nsimple_test'

print_desc "a without permission, b with permission"
chmod -x /tmp/a/simple_test; chmod +x /tmp/b/simple_test;
assert 'unset PATH\nexport PATH="/tmp/a:/tmp/b"\nsimple_test'
assert 'unset PATH\nexport PATH="/tmp/b:/tmp/a"\nsimple_test'


# Redirect
## Redirecting output
assert 'echo hello >hello.txt' 'hello.txt'
assert 'echo hello >f1>f2>f3' 'f1' 'f2' 'f3'

## Redirecting input
assert 'cat <Makefile'
echo hello >f1
echo world >f2
echo 42Tokyo >f3
assert 'cat <f1<f2<f3'
rm -f f1 f2 f3
assert 'cat <hoge'

## Appending Redirected output
assert 'pwd >>pwd.txt' 'pwd.txt'
assert 'pwd >>pwd.txt \n pwd >>pwd.txt' 'pwd.txt'

## Here Document
assert 'cat <<EOF\nhello\nworld\nEOF\nNOPRINT'
assert 'cat <<EOF<<eof\nhello\nworld\nEOF\neof\nNOPRINT'
assert 'cat <<EOF\nhello\nworld'
assert 'cat <<E"O"F\nhello\nworld\nEOF\nNOPRINT'
assert 'cat <<EOF   \n$USER\n$NO_SUCH_VAR\n$FOO$BAR\nEOF'
assert 'cat <<"EOF" \n$USER\n$NO_SUCH_VAR\n$FOO$BAR\nEOF'
assert 'cat <<EO"F" \n$USER\n$NO_SUCH_VAR\n$FOO$BAR\nEOF'
(
	print_desc 'export EOF="eof"'
	export EOF="eof"
	assert 'cat <<$EOF         \neof\n$EOF\nEOF'
	assert 'cat <<"$EOF"       \neof\n$EOF\nEOF'
)

# Pipe
assert 'cat Makefile | grep minishell'
assert 'cat | cat | ls\n\n'

# Expand Variable
assert 'echo $USER'
assert 'echo $USER$PATH$TERM'
assert 'echo "$USER  $PATH   $TERM"'

# ----------------------------------------------------------------- 
cleanup

if [ -f error.log ]; then
	echo -e $RED"Some tests have failed. Please review the error.log for more information."$RESET
fi