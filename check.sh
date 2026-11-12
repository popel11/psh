if [ -n "$1" ]; then
	g++ -c -o /dev/null -Wall -Wextra -pedantic -pipe $1
else
	for f in *.cpp; do
		g++ -c -o /dev/null -Wall -Wextra -pedantic -pipe $f
	done
fi
