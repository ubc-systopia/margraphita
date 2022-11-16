# Systopia Graph Storage Project

To build this project, go to the build directory, and update the paths in paths.sh to reflect your setup.

Then execute the runner.sh script.

# Requirements
- g++11
```
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install g++-11
```
- [cmake](https://cmake.org/install/)
- [Boost](https://www.boost.org/)  (1.80.0 or higher)

We recommend either of the following Kronecker Generators:

- [Smooth Kronecker Generator](https://github.com/dmargo/smooth_kron_gen)
- [PaRMAT](https://github.com/farkhor/PaRMAT)

The knonecker graph generation script currently uses PaRMAT. You can easily change it to use Smooth Kronecker Gernerator.
