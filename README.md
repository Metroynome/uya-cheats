# deadlocked-cheats

Sample project showing how to use libdl to create cheat codes for Ratchet and Clank: Up Your Arsenal
## Build with docker
### Clone Repos
To build with docker you need to install docker, then clone these repos:
 - To make it easy, just clone the repos into the same directory.
 - First is the `Horizon-UYA-Patch` Repo
    - This repo is required due to it using the Ratchet and Clank: UYA library, `libuya`
 - Second is the actual `uya-cheats` repo itself.
```sh
git clone https://github.com/Horizon-Private-Server/horizon-uya-patch
git clone https://github.com/Metroynome/uya-cheats.git
```
### Get PS2SDK via Docker
You will need to run this command in the top directory that `horizon-uya-patch` and `uya-cheats` is in.  `uya-cheats` requires access to both directories.
```sh
docker pull ps2dev/ps2dev:v1.2.0
docker run -it --rm -v "$PWD\:/src" ps2dev/ps2dev:v1.2.0
cd src
```

### Install libuya
```sh
cd ./horizon-uya-patch/
./docker-init.sh
make install
```

### Back to uya-cheats!
```sh
cd ..
cd ./uya-cheats/
```

## Create the cheat!
To create the cheats, just cd into a directory and then run `make clean && make`
Example:
```sh
cd ./uya-walk-through-walls/
make clean && make
```
The output file will be placed into the /bin/ directory.


Make it:
```sh
make clean && make
```
