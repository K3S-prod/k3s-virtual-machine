# k3s-virtual-machine
Virtual machine implementation for Xiaomi virtual machines course

# Building from source
```shell
sudo ./bootstrap.sh
mkdir build && cd build
cmake .. -G Ninja
ninja
```

# Run benchmarks:
```shell
cd build
./bin/asm ../benchmarks/fibbonaci.k3s fibbonaci.k3sm
./bin/interpreter fibbonaci.k3sm 

./bin/asm ../benchmarks/mean.k3s mean.k3sm
./bin/interpreter mean.k3sm 
```
