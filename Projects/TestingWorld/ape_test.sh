BASE=/cr/users/filip/Projects/TestingWorld

cat << EOF > ${BASE}/aperc
[DEFAULT]
base = $BASE/
build = /tmp/$(whoami)/ape-build/
 
[ape]
mirrors = de mx
jobs = 1
logs = %(base)s/logs
distfiles = %(base)s/distfiles 

# force GNU compilers for older systems
cc = gcc
cxx = g++

EOF
echo ">>> The file 'aperc' was created."
APERC="${BASE}/aperc"
