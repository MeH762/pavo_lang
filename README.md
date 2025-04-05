# Pavo Lang

Pavo Lang is a mini language interpreter written in C. This project aims to provide a simple and efficient interpreter for a custom-designed language called Pavo.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Features](#features)

## Installation

To install Pavo Lang, follow these steps:

1. Clone the repository:
    ```sh
    git clone https://github.com/MeH762/pavo_lang.git
    cd pavo_lang
    ```

2. Compile the source code:
    ```sh
    gcc ast.c main.c parser.c lexer.c map.c -o pavo
    ```

## Usage

To run the Pavo Lang interpreter, use the following command:
```sh
./pavo
```

## Features

Variables:

```sh
let x: num = 12;
let a: bool = true;
```
It also has type inference for these types:
```sh
let y := 5;
```

Mathematical operations: +, -, /, *, **

```sh
let a := 2;
let b := 3;

println a+b;
```

It also supports more complex expressions, like:

```sh
let c: num = (a+b)/(a-2) * (8**2);
```

Control flow:

```sh
//works with bools, also a num evaluates to true if its != 0
if expr {
    //do something
}

//loops:
for i : 0->5 {//0,1,2,3,4

}
```

Comparison checks:
```sh
let p: bool = a<b;
p = 1==1;
```
