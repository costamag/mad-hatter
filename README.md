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

## 📑 Table of Contents
- [🚀 Features](#-features)
- [📦 Installation](#-installation)
- [📖 Related Papers](#-related-papers)
- [✍️ Citing Mad Hatter](#️-citing-mad-hatter)

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

---
<<<<<<< HEAD

=======
>>>>>>> e2a3b08 (docs: literature)
## 📖 Related Papers

Mad Hatter is based on research in logic resynthesis and SPFD techniques.  
Some key references:

<<<<<<< HEAD
- A. Costamagna, A. Mishchenko, S. Chatterjee, and G. De Micheli,  
  *Symmetry-Based Synthesis for Interpretable Boolean Evaluation*,  
  2025 38th International Conference on VLSI Design and 2024 23rd International Conference on Embedded Systems (VLSID), Bangalore, India, 2025, pp. 374-379.  
  [10.1109/VLSID64188.2025.00077](https://doi.org/10.1109/VLSID64188.2025.00077)

- A. Costamagna, A. Mishchenko, S. Chatterjee, and G. De Micheli,   
  *An Enhanced Resubstitution Algorithm for Area-Oriented Logic Optimization*,  
  2024 IEEE International Symposium on Circuits and Systems (ISCAS), Singapore, Singapore, 2024, pp. 1-5.  
  [DOI: 10.1109/ISCAS58744.2024.10558264](https://doi.org/10.1109/ISCAS58744.2024.10558264)

- A. Costamagna, A. Tempia Calvino, A. Mishchenko and G. De Micheli,   
  *Area-Oriented Resubstitution For Networks of Look-Up Tables*,  
  2025 IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems (TCAD), vol. 44, no. 7, pp. 2571-2584, July 2025.  
  [DOI: 10.1109/TCAD.2025.3525617](https://doi.org/10.1109/TCAD.2025.3525617) [🏆 Best Paper Award at IWLS 2024] 

- A. Costamagna, A. Tempia Calvino, A. Mishchenko and G. De Micheli,   
  *Area-Oriented Optimization After Standard-Cell Mapping*,  
  2025 Proceedings of the 30th Asia and South Pacific Design Automation Conference (ASP-DAC), pp. 1112-1119.  
  [DOI: 10.1145/3658617.3697722](https://doi.org/10.1145/3658617.3697722)

- A. Costamagna, C. Meng and G. De Micheli,   
  *SPFD-Based Delay Resynthesis*,  
  2025 21st International Conference on Synthesis, Modeling, Analysis and Simulation Methods, and Applications to Circuits Design (SMACD), Istanbul, Turkiye, 2025, pp. 1-4.  
  [DOI: 10.1109/SMACD65553.2025.11091999](https://doi.org/10.1109/SMACD65553.2025.11091999)  [🏆 Best Paper Award] 

- A. Costamagna, X. Xu, G. De Micheli and D. Ruic,   
  *Lazy Man’s Resynthesis For Glitching-Aware Power Minimization*,  
  2025 IEEE 28th International Symposium on Design and Diagnostics of Electronic Circuits and Systems (DDECS), Lyon, France, 2025, pp. 92-98.  
  [DOI: 10.1109/DDECS63720.2025.11006815](https://doi.org/10.1109/DDECS63720.2025.11006815) 
=======
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
>>>>>>> e2a3b08 (docs: literature)
