<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 6 : Traitement des données <!-- omit in toc -->

# Table des matières <!-- omit in toc -->

- [Commande pour les labos](#commande-pour-les-labos)
  - [Building](#building)
  - [Run on DE1-SoC](#run-on-de1-soc)
  - [Seral port communication](#seral-port-communication)
  - [SSH connection](#ssh-connection)
  - [Redirect output to file](#redirect-output-to-file)
  - [Get file from DE1-SoC](#get-file-from-de1-soc)
  - [Send file to DE1-SoC](#send-file-to-de1-soc)
  - [Remote debug](#remote-debug)
    - [Run gdbserver on DE1-SoC](#run-gdbserver-on-de1-soc)

___

# Commande pour les labos

## Building

### CMake <!-- omit in toc -->

For DE1-SoC:

```bash
rm -rf build
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/home/reds/de1soc-sdk/share/buildroot/toolchainfile.cmake ..
```

For Local:

```bash
rm -rf build
mkdir build
cd build
cmake ..
```

### Make <!-- omit in toc -->

```bash
make
```

## Run on DE1-SoC

```bash
# Deploie le programme sur la carte
/media/sf_Partague_VM_Reds_2025/PTR/ptr_labo7/de1soc.sh deploy main

# Deploie puis lance le programme sur la carte
../de1soc.sh run main

# Termine le programme sur la carte
../de1soc.sh kill main
```

## Seral port communication

```bash
sudo picocom /dev/ttyUSB0 -−b 115200
```

MDP : heig

## SSH connection

```bash
ssh root@192.168.0.2
```

## Redirect output to file

```bash	
./prog > result.txt
```

## Get file from DE1-SoC

```bash
scp root@192.168.0.2:/root/audio_after.wav .
scp root@192.168.0.2:/root/audio_before.wav .
```

### Get perf results <!-- omit in toc -->

```bash
scp root@192.168.0.2:/root/perf_audio_results.txt .
scp root@192.168.0.2:/root/perf_video_results_0.txt .
scp root@192.168.0.2:/root/perf_video_results_1.txt .
scp root@192.168.0.2:/root/perf_video_results_3.txt .
```

## Send file to DE1-SoC

```bash
scp output_video.raw root@192.168.0.2:/root/output_video.raw
```

## Remote debug

### Run gdbserver on DE1-SoC

```bash
gdbserver --multi :2345 ./main
```

taskset -pc 0 $$
stress --cpu 8 --io 4 --vm 1 --timeout 60s

___


</div>