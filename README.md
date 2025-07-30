<p align="center">
  <img src="assets/logo.svg" alt="Mad Hatter Logo" width="200"/>
</p>

<h1 align="center">Mad Hatter Resynthesizer</h1>

<p align="center">
  <em>A modern logic resynthesis tool powered by <a href="https://github.com/lsils/mockturtle">mockturtle</a></em>
</p>

<p align="center">
  <a href="https://github.com/costamag/mad-hatter/actions">
    <img src="https://github.com/costamag/mad-hatter/actions/workflows/ci.yml/badge.svg" alt="Build Status">
  </a>
  <a href="https://github.com/costamag/mad-hatter/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/costamag/mad-hatter.svg" alt="License">
  </a>
  <a href="https://github.com/costamag/mad-hatter/stargazers">
    <img src="https://img.shields.io/github/stars/costamag/mad-hatter.svg" alt="Stars">
  </a>
</p>

---

## 🚀 Features

- SPFD-based logic resynthesis
- Powered by [mockturtle](https://github.com/lsils/mockturtle)  
- Interactive CLI with [replxx](https://github.com/AmokHuginnsson/replxx)  
- Cross-platform build with CMake  

---

## 📦 Installation

```bash
git clone --recurse-submodules https://github.com/costamag/mad-hatter.git
cd mad-hatter
mkdir build && cd build
cmake ..
make

