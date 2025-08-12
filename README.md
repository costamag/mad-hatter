<p align="center">
  <img src="assets/logo.svg" alt="Rinox Logo" width="200"/>
</p>

<h1 align="center">
  <strong>RINOX</strong> = <strong>R</strong>esynthesis for <strong>I</strong>nterconnect and <strong>N</strong>ode <strong>O</strong>ptimization with e<strong>X</strong>ploration
</h1>

<p align="center">
  <em>A modern logic resynthesis tool powered by <a href="https://github.com/lsils/mockturtle">mockturtle</a></em>
</p>

<p align="center">
  <a href="https://github.com/costamag/rinox/actions">
    <img src="https://github.com/costamag/rinox/actions/workflows/ci.yml/badge.svg" alt="Build Status">
  </a>
  <a href="https://github.com/costamag/rinox/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/costamag/rinox.svg" alt="License">
  </a>
  <a href="https://github.com/costamag/rinox/stargazers">
    <img src="https://img.shields.io/github/stars/costamag/rinox.svg" alt="Stars">
  </a>
</p>

---

## 📑 Table of Contents
- [🚀 Features](#-features)
- [📦 Installation](#-installation)
- [📖 Related Papers](#-related-papers)
- [✍️ Citing Rinox](#️-citing-rinox)

---

## 🧠 What is Rinox?

Rinox is a high-performance logic resynthesis tool for optimizing interconnects and nodes in digital circuits.  
It integrates advanced SPFD-based algorithms with [mockturtle](https://github.com/lsils/mockturtle) and offers an interactive command-line interface for exploring optimization strategies.

---

## 🚀 Features

- SPFD-based logic resynthesis  
- Powered by [mockturtle](https://github.com/lsils/mockturtle)  
- Interactive CLI with [replxx](https://github.com/AmokHuginnsson/replxx)  
- Cross-platform build with CMake  

---

## 📦 Installation

```bash
git clone --recurse-submodules https://github.com/costamag/rinox.git
cd rinox
mkdir build && cd build
cmake ..
make
```

---

## 💡 Why "Rinox"?

The name **Rinox** is both technical and personal.  

- **Technical**: It stands for **R**esynthesis for **I**nterconnect and **N**ode **O**ptimization with e**X**ploration, reflecting the tool's focus on advanced logic resynthesis techniques.  
- **Personal**: It is a tribute to **Rino**, a skilled carpenter whose dedication and craftsmanship inspired this project. The **X** also marks the tenth major rewrite of the code before making it public.


---

## 📖 Related Papers

Rinox is based on research in logic resynthesis and SPFD techniques.  
Some key references:

<details>
<summary>Click to expand</summary>

- A. Costamagna, A. Mishchenko, S. Chatterjee, and G. De Micheli,  
  *Symmetry-Based Synthesis for Interpretable Boolean Evaluation*,  
  VLSID 2025, Bangalore, India, pp. 374–379.  
  [DOI: 10.1109/VLSID64188.2025.00077](https://doi.org/10.1109/VLSID64188.2025.00077)

- A. Costamagna, A. Mishchenko, S. Chatterjee, and G. De Micheli,  
  *An Enhanced Resubstitution Algorithm for Area-Oriented Logic Optimization*,  
  ISCAS 2024, Singapore, pp. 1–5.  
  [DOI: 10.1109/ISCAS58744.2024.10558264](https://doi.org/10.1109/ISCAS58744.2024.10558264)

- A. Costamagna, A. Tempia Calvino, A. Mishchenko, and G. De Micheli,  
  *Area-Oriented Resubstitution For Networks of Look-Up Tables*,  
  IEEE TCAD, vol. 44, no. 7, pp. 2571–2584, July 2025.  
  [DOI: 10.1109/TCAD.2025.3525617](https://doi.org/10.1109/TCAD.2025.3525617)  
  **🏆 Best Paper Award — IWLS 2024**

- A. Costamagna, A. Tempia Calvino, A. Mishchenko, and G. De Micheli,  
  *Area-Oriented Optimization After Standard-Cell Mapping*,  
  ASP-DAC 2025, pp. 1112–1119.  
  [DOI: 10.1145/3658617.3697722](https://doi.org/10.1145/3658617.3697722)

- A. Costamagna, C. Meng, and G. De Micheli,  
  *SPFD-Based Delay Resynthesis*,  
  SMACD 2025, Istanbul, Turkiye, pp. 1–4.  
  [DOI: 10.1109/SMACD65553.2025.11091999](https://doi.org/10.1109/SMACD65553.2025.11091999)  
  **🏆 Best Paper Award — SMACD 2025**

- A. Costamagna, X. Xu, G. De Micheli, and D. Ruic,  
  *Lazy Man’s Resynthesis For Glitching-Aware Power Minimization*,  
  DDECS 2025, Lyon, France, pp. 92–98.  
  [DOI: 10.1109/DDECS63720.2025.11006815](https://doi.org/10.1109/DDECS63720.2025.11006815)

</details>
