# 安装依赖库
sudo apt install m4
sudo apt install flex
sudo apt install bison
# 下载gmp库 https://gmplib.org/#DOWNLOAD
tar xvf gmp-6.3.0.tar.xz
cd gmp-6.3.0
./configure
make && make check
sudo make install
# 下载pbc库 https://crypto.stanford.edu/pbc/download.html
tar xvf pbc-0.5.14.tar.gz
cd pbc-0.5.15
./configure
make 
sudo make install
------------------
# 添加动态链接目录
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH