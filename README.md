# Compile Linux

## Debian/Ubuntu

```
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install gcc-multilib g++-multilib
sudo apt install libsdl2-dev
sudo apt install libsdl2-dev:i386
sudo apt install libpng-dev
sudo apt install libfreetype6-dev
sudo apt install libfreetype6-dev:i386
sudo apt install python3
sudo apt install python3-pip
sudo apt install meson

setup.py -b EUR_MQD
meson setup linux --cross-file x86-linux-gnu
cd linux
ninja
```

# GLideN64 [![Github Badge]][Workflow]

*A next generation* ***Graphics Plugin*** *for* ***N64*** *emulators.*

---

## Continuous Integration

**CI** builds have the latest `features` / `fixes` , are generally <br>
stable, but may introduce **bugs** and have *incomplete translations* .

<br>

To obtain **CI** builds for the `mupen64plus` & <br>
`zilmar-spec` emulators do the following :

##### With Github

Download them from the latest **[Workflow]** .

##### Without Github

Download them from the latest **[Release]** .

<br>

##### Version

*Choose between `32-bit` / `64-bit`* <br>
*according to your emulator version.*

##### Earlier Builds

*For earlier builds you will have to log in and <br>
download them from an older* ***[Workflow]*** *.*


<!----------------------------------------------------------------------------->

[Wiki]: https://github.com/gonetz/GLideN64/wiki

[Release]: https://github.com/gonetz/GLideN64/releases/tag/github-actions
[Workflow]: https://github.com/gonetz/GLideN64/actions?query=branch%3Amaster

[Github Badge]: https://github.com/gonetz/GLideN64/actions/workflows/build.yml/badge.svg?branch=master
