sudo apt update
sudo apt install gcc-arm-none-eabi

git clone https://github.com/bugbuster-dev/ChibiOS.git
git clone https://github.com/bugbuster-dev/wasm-micro-runtime.git wamr

make

