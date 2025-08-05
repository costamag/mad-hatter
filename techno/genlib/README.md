# Technology Libraries for mad-hatter

This folder contains technology libraries used by `mad-hatter` for 
technology mapping, logic synthesis, and analysis.  

## Directory Structure

```
techno/
├── genlib/
│ ├── asap7.genlib # Example standard cell library
│ ├── nangate45.genlib # Another common library
│ └── custom.genlib # User-provided library
└── liberty/
├── asap7.lib
└── nangate45.lib
```

- **genlib/**  
  Contains `.genlib` files describing gate-level libraries (used for mapping).  
  You can add your own `.genlib` files here.

- **liberty/**  
  Contains `.lib` (Liberty format) files, typically used for timing and 
  technology information.

## Adding Your Own Library
1. Place your `.genlib` file into the `techno/genlib/` folder.  
2. Make sure the file follows the `.genlib` format (see examples provided).  
3. When running tools, you can reference it by filename (no need for full path) 
   if using the CMake integration.

## Example Usage
```bash
./mad-hatter-cli --genlib asap7.genlib --verilog design.v
```