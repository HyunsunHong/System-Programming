#!/bin/bash

# initialize files
rm ./server_dir/sharing_dir/*.txt
rm ./client_dir/client1_exe/sharing_dir1/*.txt
rm ./client_dir/client2_exe/sharing_dir2/*.txt
rm ./client_dir/client3_exe/sharing_dir3/*.txt

cat > ./server_dir/sharing_dir/server.txt << _EOF_
server server server
_EOF_

cat > ./client_dir/client1_exe/sharing_dir1/a.txt << _EOF_
a a a
_EOF_

cat > ./client_dir/client1_exe/move.txt << _EOF_
move move move
_EOF_

cat > ./client_dir/client2_exe/sharing_dir2/b.txt << _EOF_
b b b
_EOF_

cat > ./client_dir/client3_exe/sharing_dir3/c.txt << _EOF_
c c c
_EOF_

# now test
./server_dir/fshare_server -p 8030 -m start -d ./server_dir/sharing_dir  &
sleep 0.1

./client_dir/client1_exe/fshare_client -p 127.0.0.1:8030 -m start -d ./client_dir/client1_exe/sharing_dir1 &
sleep 0.1
mv ./client_dir/client1_exe/move.txt ./client_dir/client1_exe/sharing_dir1
sleep 0.1

./client_dir/client2_exe/fshare_client -p 127.0.0.1:8030 -m start -d ./client_dir/client2_exe/sharing_dir2 &
sleep 0.1
mv ./client_dir/client2_exe/sharing_dir2/b.txt ./client_dir/client2_exe/sharing_dir2/bb.txt
sleep 0.1
rm ./client_dir/client2_exe/sharing_dir2/bb.txt
sleep 0.1

./client_dir/client3_exe/fshare_client -p 127.0.0.1:8030 -m start -d ./client_dir/client3_exe/sharing_dir3 &
sleep 0.1
mv ./client_dir/client3_exe/sharing_dir3/c.txt ./client_dir/client3_exe/
sleep 0.1
touch ./client_dir/client3_exe/sharing_dir3/d.txt
sleep 0.1
cat > ./client_dir/client2_exe/sharing_dir2/e.txt << _EOF_
e e e
_EOF_

./client_dir/client3_exe/fshare_client -p 127.0.0.1:8030 -m stop
sleep 0.1
./server_dir/fshare_server -p 8030 -m stop
